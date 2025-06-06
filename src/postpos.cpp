/*------------------------------------------------------------------------------
* postpos.c : post-processing positioning
*
*          Copyright (C) 2007-2016 by T.TAKASU, All rights reserved.
*
* version : $Revision: 1.1 $ $Date: 2008/07/17 21:48:06 $
* history : 2007/05/08  1.0  new
*           2008/06/16  1.1  support binary inputs
*           2009/01/02  1.2  support new rtk positioing api
*           2009/09/03  1.3  fix bug on combined mode of moving-baseline
*           2009/12/04  1.4  fix bug on obs data buffer overflow
*           2010/07/26  1.5  support ppp-kinematic and ppp-static
*                            support multiple sessions
*                            support sbas positioning
*                            changed api:
*                                postpos()
*                            deleted api:
*                                postposopt()
*           2010/08/16  1.6  fix bug sbas message synchronization (2.4.0_p4)
*           2010/12/09  1.7  support qzss lex and ssr corrections
*           2011/02/07  1.8  fix bug on sbas navigation data conflict
*           2011/03/22  1.9  add function reading g_tec file
*           2011/08/20  1.10 fix bug on freez if solstatic=single and combined
*           2011/09/15  1.11 add function reading stec file
*           2012/02/01  1.12 support keyword expansion of rtcm ssr corrections
*           2013/03/11  1.13 add function reading otl and erp data
*           2014/06/29  1.14 fix problem on overflow of # of satellites
*           2015/03/23  1.15 fix bug on ant type replacement by rinex header
*                            fix bug on combined filter for moving-base mode
*           2015/04/29  1.16 fix bug on reading rtcm ssr corrections
*                            add function to read satellite fcb
*                            add function to read stec and troposphere file
*                            add keyword replacement in dcb, erp and ionos file
*           2015/11/13  1.17 add support of L5 antenna phase center paramters
*                            add *.stec and *.trp file for ppp correction
*           2015/11/26  1.18 support opt->freqopt(disable L2)
*           2016/01/12  1.19 add carrier-phase bias correction by ssr
*           2016/07/31  1.20 fix error message problem in rnx2rtkp
*           2016/08/29  1.21 suppress warnings
*           2016/10/10  1.22 fix bug on identification of file fopt->blq
*           2017/06/13  1.23 add smoother of velocity solution
*-----------------------------------------------------------------------------*/
#include "rtklib.h"

#define MIN(x,y)    ((x)<(y)?(x):(y))
#define SQRT(x)     ((x)<=0.0||(x)!=(x)?0.0:sqrt(x))

#define MAXPRCDAYS  100          /* max days of continuous processing */
#define MAXINFILE   1000         /* max number of input files */

/* constants/global variables ------------------------------------------------*/

static pcvs_t pcvss={0};        /* satellite antenna parameters */
static pcvs_t pcvsr={0};        /* receiver antenna parameters */
static obs_t obss={0};          /* observation data */
static nav_t navs={0};          /* navigation data */
static sbs_t sbss={0};          /* sbas messages */
static lex_t lexs={0};          /* lex messages */
static sta_t stas[MAXRCV];      /* station infomation */
static int nepoch=0;            /* number of observation epochs */
static int iobsu =0;            /* current rover observation data index */
static int iobsr =0;            /* current reference observation data index */
static int isbs  =0;            /* current sbas message index */
static int ilex  =0;            /* current lex message index */
static int revs  =0;            /* analysis direction (0:forward,1:backward) */
static int prgbar = 0;          /* progress bar / 进度条 */
static int aborts=0;            /* abort status */
static sol_t *solf;             /* forward solutions */
static sol_t *solb;             /* backward solutions */
static double *rbf;             /* forward base positions */
static double *rbb;             /* backward base positions */
static int isolf=0;             /* current forward solutions index */
static int isolb=0;             /* current backward solutions index */
static char proc_rov [64]="";   /* rover for current processing */
static char proc_base[64]="";   /* base station for current processing */
static char rtcm_file[1024]=""; /* rtcm data file */
static char rtcm_path[1024]=""; /* rtcm data path */
static rtcm_t rtcm;             /* rtcm control struct */
static FILE *fp_rtcm=NULL;      /* rtcm data file pointer */
static char outsppfile[1024];

/* 初始化重要的结构体 */
void init_nav(nav_t* nav) 
{
    memset(nav, 0, sizeof(nav_t)); // 清空所有内容

    // 初始化 ion_bdsk9 成员
    nav->ion_bdsk9 = (BDSSH*)malloc(sizeof(BDSSH));
    if (!nav->ion_bdsk9) {
        fprintf(stderr, "内存分配失败（ion_bdsk9）\n");
        exit(EXIT_FAILURE);
    }
    // 你可以在这里继续初始化 nav 其他指针成员（如 eph, geph 等）根据需要
    nav->eph = NULL; nav->n = nav->nmax = 0;
    nav->geph = NULL; nav->ng = nav->ngmax = 0;
    nav->seph = NULL; nav->ns = nav->nsmax = 0;
    nav->galcode = 1;  /* ? 1:I/Nav, 2:FNav */
    nav->obstsys = TSYS_GPS;
    //nav->ion_bdsk9 = new BDSSH(); BDSSH改写为结构体格式
    nav->ion_bdsk9->BrdIonCoefNum = 0;// the parameter number for broadcast [9 in default] 
    nav->ion_bdsk9->bds_ion.nk8 = 0;
    nav->ion_bdsk9->bds_ion.nk14 = 0;
    nav->ion_bdsk9->bds_ion.nsh9 = 0;// the total parameter number for SH resolution [26 in default]
    nav->igmasta = -1;
}
void init_obs(obs_t* obs)
{
    memset(obs, 0, sizeof(obs_t)); // 清空所有内容
    obs->data = NULL; obs->n = obs->nmax = 0;
    // 初始化成员
    

    // 你可以在这里继续初始化 data 其他指针成员,根据需要
}
/* show message and check break ----------------------------------------------*/
/* 输出内容，如果基准站或流动站有信息，顺便输出一下 */
static int checkbrk(const char *format, ...)
{
    va_list arg;
    char buff[1024],*p=buff;
    // 如果format为空，直接返回
    if (!*format) return showmsg("");
    // 将format格式化存入buff
    va_start(arg,format);
    p+=vsprintf(p,format,arg);
    va_end(arg);
    //如果基准站（proc_base）和流动站（proc_rov）都有信息，就顺便都输出，如果只有其中一个有信息，就只输出一个
    if (*proc_rov&&*proc_base) sprintf(p," (%s-%s)",proc_rov,proc_base);
    else if (*proc_rov ) sprintf(p," (%s)",proc_rov );
    else if (*proc_base) sprintf(p," (%s)",proc_base);
    return showmsg(buff);
}
/* output reference position -------------------------------------------------*/
static void outrpos(FILE *fp, const double *r, const solopt_t *opt)
{
    double pos[3],dms1[3],dms2[3];
    const char *sep=opt->sep;
    
    trace(3,"outrpos :\n");
    
    if (opt->posf==SOLF_LLH) {
        ecef2pos(r,pos);
        if (opt->degf) {
            deg2dms(pos[0]*R2D,dms1,5);
            deg2dms(pos[1]*R2D,dms2,5);
            fprintf(fp,"%3.0f%s%02.0f%s%08.5f%s%4.0f%s%02.0f%s%08.5f%s%10.4f",
                    dms1[0],sep,dms1[1],sep,dms1[2],sep,dms2[0],sep,dms2[1],
                    sep,dms2[2],sep,pos[2]);
        }
        else {
            fprintf(fp,"%13.9f%s%14.9f%s%10.4f",pos[0]*R2D,sep,pos[1]*R2D,
                    sep,pos[2]);
        }
    }
	else if (opt->posf == SOLF_XYZ || opt->posf == SOLF_ENU) {
        fprintf(fp,"%14.4f%s%14.4f%s%14.4f",r[0],sep,r[1],sep,r[2]);
    }
}
/* output header -------------------------------------------------------------*/
static void outheader(FILE *fp, const char **file, int n, const prcopt_t *popt,
                      const solopt_t *sopt)
{
    const char *s1[]={"GPST","UTC","JST"};
    gtime_t ts,te;
    double t1,t2;
    int i,j,w1,w2;
    char s2[32],s3[32];
    
    trace(3,"outheader: n=%d\n",n);
    
    if (sopt->posf==SOLF_NMEA||sopt->posf==SOLF_STAT) {
        return;
    }
    if (sopt->outhead) {
        //if (!*sopt->prog) {
        //    fprintf(fp,"%s program   : RTKLIB ver.%s\n",COMMENTH,VER_RTKLIB);
        //}
        //else {
        //    fprintf(fp,"%s program   : %s\n",COMMENTH,sopt->prog);
        //}
        for (i=0;i<n;i++) {
           // fprintf(fp,"%s inp file  : %s\n",COMMENTH,file[i]);
        }
        for (i=0;i<obss.n;i++)    if (obss.data[i].rcv==1) break;
        for (j=obss.n-1;j>=0;j--) if (obss.data[j].rcv==1) break;
        if (j<i) {fprintf(fp,"\n%s no rover obs data\n",COMMENTH); return;}
        ts=obss.data[i].time;
        te=obss.data[j].time;
        t1=time2gpst(ts,&w1);
        t2=time2gpst(te,&w2);
        if (sopt->times>=1) ts=gpst2utc(ts);
        if (sopt->times>=1) te=gpst2utc(te);
        if (sopt->times==2) ts=timeadd(ts,9*3600.0);
        if (sopt->times==2) te=timeadd(te,9*3600.0);
        time2str(ts,s2,1);
        time2str(te,s3,1);
        fprintf(fp,"%s obs start : %s %s (week%04d %8.1fs)\n",COMMENTH,s2,s1[sopt->times],w1,t1);
        fprintf(fp,"%s obs end   : %s %s (week%04d %8.1fs)\n",COMMENTH,s3,s1[sopt->times],w2,t2);
    }
    if (sopt->outopt) {
        outprcopt(fp,popt);
    }
    if (PMODE_DGPS<=popt->mode&&popt->mode<=PMODE_FIXED&&popt->mode!=PMODE_MOVEB) {
        fprintf(fp,"%s ref pos   :",COMMENTH);
        outrpos(fp,popt->rb,sopt);
        fprintf(fp,"\n");
    }
    if (sopt->outhead||sopt->outopt) fprintf(fp,"%s\n",COMMENTH);
    
    outsolhead(fp,sopt);
}
/* search next observation data index ----------------------------------------*/
static int nextobsf(const obs_t *obs, int *i, int rcv)
{
    double tt;
    int n;
    
    for (;*i<obs->n;(*i)++) if (obs->data[*i].rcv==rcv) break;
    for (n=0;*i+n<obs->n;n++) {
        tt=timediff(obs->data[*i+n].time,obs->data[*i].time);
        if (obs->data[*i+n].rcv!=rcv||tt>DTTOL) break;
    }
    return n;
}
static int nextobsb(const obs_t *obs, int *i, int rcv)
{
    double tt;
    int n;
    
    for (;*i>=0;(*i)--) if (obs->data[*i].rcv==rcv) break;
    for (n=0;*i-n>=0;n++) {
        tt=timediff(obs->data[*i-n].time,obs->data[*i].time);
        if (obs->data[*i-n].rcv!=rcv||tt<-DTTOL) break;
    }
    return n;
}
/* update rtcm ssr correction ------------------------------------------------*/
static void update_rtcm_ssr(gtime_t time)
{
    char path[1024];
    int i;
    
    /* open or swap rtcm file */
    reppath(rtcm_file,path,time,"","");
    
    if (strcmp(path,rtcm_path)) {
        strcpy(rtcm_path,path);
        
        if (fp_rtcm) fclose(fp_rtcm);
        fp_rtcm=fopen(path,"rb");
        if (fp_rtcm) {
            rtcm.time=time;
            input_rtcm3f(&rtcm,fp_rtcm);
            trace(2,"rtcm file open: %s\n",path);
        }
    }
    if (!fp_rtcm) return;
    
    /* read rtcm file until current time */
    while (timediff(rtcm.time,time)<1E-3) {
        if (input_rtcm3f(&rtcm,fp_rtcm)<-1) break;
        
        /* update ssr corrections */
        for (i=0;i<MAXSAT;i++) {
            if (!rtcm.ssr[i].update||
                rtcm.ssr[i].iod[0]!=rtcm.ssr[i].iod[1]||
                timediff(time,rtcm.ssr[i].t0[0])<-1E-3) continue;
            navs.ssr[i]=rtcm.ssr[i];
            rtcm.ssr[i].update=0;
        }
    }
}
/* input obs data, navigation messages and sbas correction -------------------*/
static int inputobs(obsd_t *obs, int solq, const prcopt_t *popt)
{
    gtime_t time={0};
    int i,nu,nr,n=0;
    
    trace(3,"infunc  : revs=%d iobsu=%d iobsr=%d isbs=%d\n",revs,iobsu,iobsr,isbs);
    
    if (0<=iobsu&&iobsu<obss.n) 
    {
        settime((time=obss.data[iobsu].time));
        //if (checkbrk("processing : %s Q=%d",time_str(time,0),solq)) {
        //    aborts=1; showmsg("aborted"); return -1;
        //}
    }
    if (!revs) { /* input forward data */
        if ((nu=nextobsf(&obss,&iobsu,1))<=0) return -1;
        if (popt->intpref) {
            for (;(nr=nextobsf(&obss,&iobsr,2))>0;iobsr+=nr)
                if (timediff(obss.data[iobsr].time,obss.data[iobsu].time)>-DTTOL) break;
        }
        else {
            for (i=iobsr;(nr=nextobsf(&obss,&i,2))>0;iobsr=i,i+=nr)
                if (timediff(obss.data[i].time,obss.data[iobsu].time)>DTTOL) break;
        }
        nr=nextobsf(&obss,&iobsr,2);
        if (nr<=0) {
            nr=nextobsf(&obss,&iobsr,2);
        }
        for (i=0;i<nu&&n<MAXOBS*2;i++) obs[n++]=obss.data[iobsu+i];
        for (i=0;i<nr&&n<MAXOBS*2;i++) obs[n++]=obss.data[iobsr+i];
        iobsu+=nu;
        
        /* update sbas corrections */
        while (isbs<sbss.n) {
            time=gpst2time(sbss.msgs[isbs].week,sbss.msgs[isbs].tow);
            
            if (getbitu(sbss.msgs[isbs].msg,8,6)!=9) { /* except for geo nav */
                sbsupdatecorr(sbss.msgs+isbs,&navs);
            }
            if (timediff(time,obs[0].time)>-1.0-DTTOL) break;
            isbs++;
        }
        /* update lex corrections */
        while (ilex<lexs.n) {
            if (lexupdatecorr(lexs.msgs+ilex,&navs,&time)) {
                if (timediff(time,obs[0].time)>-1.0-DTTOL) break;
            }
            ilex++;
        }
        /* update rtcm ssr corrections */
        if (*rtcm_file) {
            update_rtcm_ssr(obs[0].time);
        }
    }
    else { /* input backward data */
        if ((nu=nextobsb(&obss,&iobsu,1))<=0) return -1;
        if (popt->intpref) {
            for (;(nr=nextobsb(&obss,&iobsr,2))>0;iobsr-=nr)
                if (timediff(obss.data[iobsr].time,obss.data[iobsu].time)<DTTOL) break;
        }
        else {
            for (i=iobsr;(nr=nextobsb(&obss,&i,2))>0;iobsr=i,i-=nr)
                if (timediff(obss.data[i].time,obss.data[iobsu].time)<-DTTOL) break;
        }
        nr=nextobsb(&obss,&iobsr,2);
        for (i=0;i<nu&&n<MAXOBS*2;i++) obs[n++]=obss.data[iobsu-nu+1+i];
        for (i=0;i<nr&&n<MAXOBS*2;i++) obs[n++]=obss.data[iobsr-nr+1+i];
        iobsu-=nu;
        
        /* update sbas corrections */
        while (isbs>=0) {
            time=gpst2time(sbss.msgs[isbs].week,sbss.msgs[isbs].tow);
            
            if (getbitu(sbss.msgs[isbs].msg,8,6)!=9) { /* except for geo nav */
                sbsupdatecorr(sbss.msgs+isbs,&navs);
            }
            if (timediff(time,obs[0].time)<1.0+DTTOL) break;
            isbs--;
        }
        /* update lex corrections */
        while (ilex>=0) {
            if (lexupdatecorr(lexs.msgs+ilex,&navs,&time)) {
                if (timediff(time,obs[0].time)<1.0+DTTOL) break;
            }
            ilex--;
        }
    }
    return n;
}
/* carrier-phase bias correction by fcb --------------------------------------*/
static void corr_phase_bias_fcb(obsd_t *obs, int n, const nav_t *nav)
{
    int i,j,k;
    
    for (i=0;i<nav->nf;i++) {
        if (timediff(nav->fcb[i].te,obs[0].time)<-1E-3) continue;
        if (timediff(nav->fcb[i].ts,obs[0].time)> 1E-3) break;
        for (j=0;j<n;j++) {
            for (k=0;k<NFREQ;k++) {
                if (obs[j].L[k]==0.0) continue;
                obs[j].L[k]-=nav->fcb[i].bias[obs[j].sat-1][k];
            }
        }
        return;
    }
}
/* carrier-phase bias correction by ssr --------------------------------------*/
static void corr_phase_bias_ssr(obsd_t *obs, int n, const nav_t *nav)
{
    double lam;
    int i,j,code;
    
    for (i=0;i<n;i++) for (j=0;j<NFREQ;j++) {
        
        if (!(code=obs[i].code[j])) continue;
        if ((lam=nav->lam[obs[i].sat-1][j])==0.0) continue;
        
        /* correct phase bias (cyc) */
        obs[i].L[j]-=nav->ssr[obs[i].sat-1].pbias[code-1]/lam;
    }
}
/* process positioning -------------------------------------------------------*/
static void procpos(FILE *fp, const prcopt_t *popt, const solopt_t *sopt,
                    int mode)
{
    gtime_t time={0},ts,te;
    sol_t sol={{0}};
    rtk_t rtk;
    obsd_t obs[MAXOBS*2]; /* for rover and base */
    double rb[3]={0};
	FILE *fpres = NULL, *fpsnr = NULL;
    int i,j,nobs,n,solstatic,pri[]={0,1,2,3,4,5,1,6};
	double dt;
	gtime_t ptime;
	int flag1, flag2, flag3;
	char filestr[1024];
	double pos[3], dr[3];

    trace(3,"procpos : mode=%d\n",mode);
    
    solstatic=((sopt->solstatic)&&(popt->mode==PMODE_STATIC||popt->mode==PMODE_PPP_STATIC));
    
    rtkinit(&rtk,popt);
    rtcm_path[0]='\0';
    
	ts = obss.data[0].time;
	te = obss.data[obss.n - 1].time;
	dt = (int)(timediff(te, ts)/100);

	strcpy(filestr, outsppfile);
	fpres = fopen(strcat(filestr, "psu_res"), "w");

	strcpy(filestr, outsppfile);
	fpsnr = fopen(strcat(filestr, "psu_snr"), "w");

	/* rover position by single point positioning */
	ecef2pos(popt->ru, pos);
	enu2ecef(pos, popt->antdel[0], dr);
	
	for (i = 0; i < 3; i++)
    {
		rtk.sol.rf[i] = dr[i] + rtk.opt.ru[i];
		rtk.opt.ru[i] = rtk.sol.rf[i];
	}
	rtk.tsys = navs.obstsys;
	rtk.sol.obstsys = navs.obstsys;
    while ((nobs=inputobs(obs,rtk.sol.stat,popt))>=0) {
        /* exclude satellites */
        for (i=n=0;i<nobs;i++) {
			rtk.sol.sat[obs[i].sat - 1] = -1;
            if ((satsys(obs[i].sat,NULL)&popt->navsys)&&
                popt->exsats[obs[i].sat-1]!=1) obs[n++]=obs[i];
        }
        if (n<=0) continue;
		ptime = timeadd(ts, prgbar*dt);
		if (!revs){
			if (timediff(obs[0].time, ptime)>0.0){
				printf("processing : %s Q=%d %3.3d%%\n", time_str(obs[0].time, 0), rtk.sol.stat, prgbar);
				fflush(stdin);
				fflush(stdout);
				prgbar++;
			}
		}
		else if(timediff(obs[0].time, ptime)<0.0){
			printf("processing : %s Q=%d %3.3d%%\n", time_str(obs[0].time, 0), rtk.sol.stat, prgbar);
			fflush(stdin);
			fflush(stdout);
		    prgbar--;
		}

        /* carrier-phase bias correction */
        if (navs.nf>0) {
            corr_phase_bias_fcb(obs,n,&navs);
        }
        else if (!strstr(popt->pppopt,"-DIS_FCB")) {
            corr_phase_bias_ssr(obs,n,&navs);
        }
        /* disable obstype unnessary */
#if 1
		int flag = 0;
		double mpl = 0.0,mpp=0.0;
		flag = 0;
/*
		for (i  = 0; i < NFREQ; i++) {
			//if (test_freq(popt->freqopt,i)) {
			if (1) {
				//for (j = 0; j < n; j++) obs[j].L[i] = obs[j].P[i] = 0.0;
				for (j = 0; j < n; j++) {
					int prn;
					if (satsys(obs[j].sat,&prn) == SYS_CMP &&prn==28){
						trace(0,"%14.3f %14.3f ", obs[j].P[i], obs[j].L[i]);
						flag = 1;
						if (obs[j].L[3] * obs[j].L[4] * obs[j].L[5]!=0.0){
							mp = obs[j].L[3] * 0.2526 + obs[j].L[2] * -0.7987 + obs[j].L[4] * 0.5461;
						}
					}
				}
			}
		}
*/
#if 0
		for (j = 0; j < n; j++) {
			int prn;
			if (satsys(obs[j].sat, &prn) == SYS_CMP &&prn == 25){
				if (obs[j].L[2] * obs[j].L[3] * obs[j].L[4] != 0.0){
					double lam1 = CLIGHT / FREQ1;
					double lam5 = CLIGHT / FREQ5;
					double lamb3 = CLIGHT / FREQ3_CMP;
					double n = sqrt((lam1*lam1 - lamb3*lamb3)*(lam1*lam1 - lamb3*lamb3) + (lam1*lam1 - lam5*lam5)*(lam1*lam1 - lam5*lam5)
						+ (lamb3*lamb3 - lam5*lam5)*(lamb3*lamb3 - lam5*lam5));
					double alpha = (lam5*lam5 - lamb3*lamb3) / n;
					double beta = (lam1*lam1 - lam5*lam5) / n;
					double gama = (lamb3*lamb3 - lam1*lam1) / n;
					mpl = obs[j].L[3] * lam1*alpha + obs[j].L[2]*lamb3 * beta + obs[j].L[4]*lam5 * gama;
					mpp = obs[j].P[3] *alpha + obs[j].P[2] * beta + obs[j].P[4]  * gama;
					trace(0, "%14.3f %14.3f %14.3f %14.3f %14.3f\n", obs[j].L[3], obs[j].L[2], obs[j].L[4], mpl,mpp);
				}
			}
		}
#endif
	/*
		for (j = 0; j < n; j++) {
			int prn;
			if (satsys(obs[j].sat, &prn) == SYS_GPS &&prn == 25){
				if (obs[j].L[0] * obs[j].L[1] * obs[j].L[2] != 0.0){
					double lam1 = CLIGHT / FREQ1;
					double lam5 = CLIGHT / FREQ5;
					double lam2 = CLIGHT / FREQ2;
					double n = sqrt((lam1*lam1 - lam2*lam2)*(lam1*lam1 - lam2*lam2) + (lam1*lam1 - lam5*lam5)*(lam1*lam1 - lam5*lam5) 
						+ (lam2*lam2 - lam5*lam5)*(lam2*lam2 - lam5*lam5));
					double alpha = (lam5*lam5 - lam2*lam2) / n;
					double beta = (lam1*lam1 - lam5*lam5) / n;
					double gama = (lam2*lam2 - lam1*lam1) / n;
					mp = obs[j].L[0] *lam1* alpha + obs[j].L[1] *lam2* beta + obs[j].L[2] * lam5*gama;
					trace(0, "%14.3f %14.3f %14.3f %14.3f\n", obs[j].L[0], obs[j].L[1],obs[j].L[2],mp);
				}
			}
		}
	*/
#endif
        if (!rtkpos(&rtk,obs,n,&navs)) continue;
        
		outsatres_single(fpres, &rtk, obs, n);
		outsatsnr_single(fpsnr, &rtk, obs, n);

        if (mode==0) { /* forward/backward */
            if (!solstatic) {
				for (i = 0; i < 3; i++) rb[i] = rtk.opt.ru[i];
				rtk.opt.mode == PMODE_SINGLE ? outsol(fp, &rtk.sol, rb, sopt) : outsol(fp, &rtk.sol, rtk.rb, sopt);
            }
			else if (rtk.sol.stat != SOLQ_NONE&&(time.time == 0 || pri[rtk.sol.stat] <= pri[sol.stat])) {
                sol=rtk.sol;
                for (i=0;i<3;i++) rb[i]=rtk.rb[i];
                if (time.time==0||timediff(rtk.sol.time,time)>0.0) {
                    time=rtk.sol.time;
                }
            }
        }
        else if (!revs) { /* combined-forward */
            if (isolf>=nepoch) return;
            solf[isolf]=rtk.sol;
            for (i=0;i<3;i++) rbf[i+isolf*3]=rtk.rb[i];
            isolf++;
        }
        else { /* combined-backward */
            if (isolb>=nepoch) return;
            solb[isolb]=rtk.sol;
            for (i=0;i<3;i++) rbb[i+isolb*3]=rtk.rb[i];
            isolb++;
        }
    }
    if (mode==0&&solstatic&&time.time!=0.0) {
        sol.time=time;
        outsol(fp,&sol,rb,sopt);
    }
    rtkfree(&rtk);
}
/* validation of combined solutions ------------------------------------------*/
static int valcomb(const sol_t *solf, const sol_t *solb)
{
    double dr[3],var[3];
    int i;
    char tstr[32];
    
    trace(3,"valcomb :\n");
    
    /* compare forward and backward solution */
    for (i=0;i<3;i++) {
        dr[i]=solf->rr[i]-solb->rr[i];
        var[i]=solf->qr[i]+solb->qr[i];
    }
    for (i=0;i<3;i++) {
        if (dr[i]*dr[i]<=16.0*var[i]) continue; /* ok if in 4-sigma */
        
        time2str(solf->time,tstr,2);
        trace(2,"degrade fix to float: %s dr=%.3f %.3f %.3f std=%.3f %.3f %.3f\n",
              tstr+11,dr[0],dr[1],dr[2],SQRT(var[0]),SQRT(var[1]),SQRT(var[2]));
        return 0;
    }
    return 1;
}
/* combine forward/backward solutions and output results ---------------------*/
static void combres(FILE *fp, const prcopt_t *popt, const solopt_t *sopt)
{
    gtime_t time={0};
    sol_t sols={{0}},sol={{0}};
    double tt,Qf[9],Qb[9],Qs[9],rbs[3]={0},rb[3]={0},rr_f[3],rr_b[3],rr_s[3];
    int i,j,k,solstatic,pri[]={0,1,2,3,4,5,1,6};
    
    trace(3,"combres : isolf=%d isolb=%d\n",isolf,isolb);
    
    solstatic=sopt->solstatic&&
              (popt->mode==PMODE_STATIC||popt->mode==PMODE_PPP_STATIC);
    
    for (i=0,j=isolb-1;i<isolf&&j>=0;i++,j--) {
        
        if ((tt=timediff(solf[i].time,solb[j].time))<-DTTOL) {
            sols=solf[i];
            for (k=0;k<3;k++) rbs[k]=rbf[k+i*3];
            j++;
        }
        else if (tt>DTTOL) {
            sols=solb[j];
            for (k=0;k<3;k++) rbs[k]=rbb[k+j*3];
            i--;
        }
        else if (solf[i].stat<solb[j].stat) {
            sols=solf[i];
            for (k=0;k<3;k++) rbs[k]=rbf[k+i*3];
        }
        else if (solf[i].stat>solb[j].stat) {
            sols=solb[j];
            for (k=0;k<3;k++) rbs[k]=rbb[k+j*3];
        }
        else {
            sols=solf[i];
            sols.time=timeadd(sols.time,-tt/2.0);
            
            if ((popt->mode==PMODE_KINEMA||popt->mode==PMODE_MOVEB)&&
                sols.stat==SOLQ_FIX) {
                
                /* degrade fix to float if validation failed */
                if (!valcomb(solf+i,solb+j)) sols.stat=SOLQ_FLOAT;
            }
            for (k=0;k<3;k++) {
                Qf[k+k*3]=solf[i].qr[k];
                Qb[k+k*3]=solb[j].qr[k];
            }
            Qf[1]=Qf[3]=solf[i].qr[3];
            Qf[5]=Qf[7]=solf[i].qr[4];
            Qf[2]=Qf[6]=solf[i].qr[5];
            Qb[1]=Qb[3]=solb[j].qr[3];
            Qb[5]=Qb[7]=solb[j].qr[4];
            Qb[2]=Qb[6]=solb[j].qr[5];
            
            if (popt->mode==PMODE_MOVEB) {
                for (k=0;k<3;k++) rr_f[k]=solf[i].rr[k]-rbf[k+i*3];
                for (k=0;k<3;k++) rr_b[k]=solb[j].rr[k]-rbb[k+j*3];
                if (smoother(rr_f,Qf,rr_b,Qb,3,rr_s,Qs)) continue;
                for (k=0;k<3;k++) sols.rr[k]=rbs[k]+rr_s[k];
            }
            else {
                if (smoother(solf[i].rr,Qf,solb[j].rr,Qb,3,sols.rr,Qs)) continue;
            }
            sols.qr[0]=(float)Qs[0];
            sols.qr[1]=(float)Qs[4];
            sols.qr[2]=(float)Qs[8];
            sols.qr[3]=(float)Qs[1];
            sols.qr[4]=(float)Qs[5];
            sols.qr[5]=(float)Qs[2];
            
            /* smoother for velocity solution */
            if (popt->dynamics) {
                for (k=0;k<3;k++) {
                    Qf[k+k*3]=solf[i].qv[k];
                    Qb[k+k*3]=solb[j].qv[k];
                }
                Qf[1]=Qf[3]=solf[i].qv[3];
                Qf[5]=Qf[7]=solf[i].qv[4];
                Qf[2]=Qf[6]=solf[i].qv[5];
                Qb[1]=Qb[3]=solb[j].qv[3];
                Qb[5]=Qb[7]=solb[j].qv[4];
                Qb[2]=Qb[6]=solb[j].qv[5];
                if (smoother(solf[i].rr+3,Qf,solb[j].rr+3,Qb,3,sols.rr+3,Qs)) continue;
                sols.qv[0]=(float)Qs[0];
                sols.qv[1]=(float)Qs[4];
                sols.qv[2]=(float)Qs[8];
                sols.qv[3]=(float)Qs[1];
                sols.qv[4]=(float)Qs[5];
                sols.qv[5]=(float)Qs[2];
            }
        }
        if (!solstatic) {
            outsol(fp,&sols,rbs,sopt);
        }
        else if (time.time==0||pri[sols.stat]<=pri[sol.stat]) {
            sol=sols;
            for (k=0;k<3;k++) rb[k]=rbs[k];
            if (time.time==0||timediff(sols.time,time)<0.0) {
                time=sols.time;
            }
        }
    }
    if (solstatic&&time.time!=0.0) {
        sol.time=time;
        outsol(fp,&sol,rb,sopt);
    }
}
/* read prec ephemeris, sbas data, lex data, tec grid and open rtcm ----------*/
static void readpreceph(char **infile, int n, const prcopt_t *prcopt,
                        nav_t *nav, sbs_t *sbs, lex_t *lex)
{
    seph_t seph0={0};
    int i;
    char *ext;
    
    trace(2,"readpreceph: n=%d\n",n);
    
    nav->ne=nav->nemax=0;
    nav->nc=nav->ncmax=0;
    nav->nf=nav->nfmax=0;
    sbs->n =sbs->nmax =0;
    lex->n =lex->nmax =0;
    
    /* read precise ephemeris files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        readsp3(infile[i],nav,0);
    }
    /* read precise clock files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        readrnxc(infile[i],nav);
    }
    /* read satellite fcb files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        if ((ext=strrchr(infile[i],'.'))&&
            (!strcmp(ext,".fcb")||!strcmp(ext,".FCB"))) {
            readfcb(infile[i],nav);
        }
    }
    /* read solution status files for ppp correction */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        if ((ext=strrchr(infile[i],'.'))&&
            (!strcmp(ext,".stat")||!strcmp(ext,".STAT")||
             !strcmp(ext,".stec")||!strcmp(ext,".STEC")||
             !strcmp(ext,".trp" )||!strcmp(ext,".TRP" ))) {
            pppcorr_read(&nav->pppcorr,infile[i]);
        }
    }
    /* read sbas message files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        sbsreadmsg(infile[i],prcopt->sbassatsel,sbs);
    }
    /* read lex message files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        lexreadmsg(infile[i],0,lex);
    }
    /* allocate sbas ephemeris */
    nav->ns=nav->nsmax=NSATSBS*2;
    if (!(nav->seph=(seph_t *)malloc(sizeof(seph_t)*nav->ns))) {
         showmsg("error : sbas ephem memory allocation");
         trace(1,"error : sbas ephem memory allocation");
         return;
    }
    for (i=0;i<nav->ns;i++) nav->seph[i]=seph0;
    
    /* set rtcm file and initialize rtcm struct */
    rtcm_file[0]=rtcm_path[0]='\0'; fp_rtcm=NULL;
    
    for (i=0;i<n;i++) {
        if ((ext=strrchr(infile[i],'.'))&&
            (!strcmp(ext,".rtcm3")||!strcmp(ext,".RTCM3"))) {
            strcpy(rtcm_file,infile[i]);
            init_rtcm(&rtcm);
            break;
        }
    }
}
/* free prec ephemeris and sbas data -----------------------------------------*/
static void freepreceph(nav_t *nav, sbs_t *sbs, lex_t *lex)
{
    int i;
    
    trace(3,"freepreceph:\n");
    
    free(nav->peph); nav->peph=NULL; nav->ne=nav->nemax=0;
    free(nav->pclk); nav->pclk=NULL; nav->nc=nav->ncmax=0;
    free(nav->fcb ); nav->fcb =NULL; nav->nf=nav->nfmax=0;
    free(nav->seph); nav->seph=NULL; nav->ns=nav->nsmax=0;
    free(sbs->msgs); sbs->msgs=NULL; sbs->n =sbs->nmax =0;
    free(lex->msgs); lex->msgs=NULL; lex->n =lex->nmax =0;
    for (i=0;i<nav->nt;i++) {
        free(nav->tec[i].data);
        free(nav->tec[i].rms );
    }
    free(nav->tec ); nav->tec =NULL; nav->nt=nav->ntmax=0;
    
    if (fp_rtcm) fclose(fp_rtcm);
    free_rtcm(&rtcm);
}
/* read obs and nav data -----------------------------------------------------*/
static int readobsnav(gtime_t ts, gtime_t te, double ti, const char **infile,
                      const int *index, int n, const prcopt_t *prcopt,
                      obs_t *obs, nav_t *nav, sta_t *sta)
{
    int i,j,ind=0,nobs=0,rcv=1;
    
    trace(3,"readobsnav: ts=%s n=%d\n",time_str(ts,0),n);
    // 初始化
    init_nav(&navs);
    init_obs(&obss);
    nav->galfreq = prcopt->freqopt;
    nepoch=0;
    
    for (i=0;i<n;i++) {
        if (checkbrk("")) return 0;
        
        if (index[i]!=ind) {
            if (obs->n>nobs) rcv++;
            ind=index[i]; nobs=obs->n; 
        }
        /* read rinex obs and nav file/ 读文件具体函数 */
        if (readrnxt(infile[i],rcv,ts,te,ti,prcopt->rnxopt[rcv<=1?0:1],obs,nav,
                     rcv<=2?sta+rcv-1:NULL)<0) {
            checkbrk("error : insufficient memory");
            trace(1,"insufficient memory\n");
            return 0;
        }
    }
	if (obs->n <= 0 && prcopt->outsat == 0) {
        checkbrk("error : no obs data");
        trace(1,"\n");
        return 0;
    }
    if (nav->n<=0&&nav->ng<=0&&nav->ns<=0) {
        checkbrk("error : no nav data");
        trace(1,"\n");
        return 0;
    }
    /* sort observation data */
    nepoch=sortobs(obs);
    
	/* copy isc index from obs to nav*/
	for (i = 0; i < 7; i++)for (j = 0; j < MAXFREQ; j++){
		nav->isci[i][j] = obs->isci[i][j];
	}
    /* delete duplicated ephemeris */
    uniqnav(nav);
    
	/* delete duplicated ion */
	double ep[6];
	if (obs->n > 0)
    {
		time2epoch(obs->data[0].time, ep);
        char iftrue = uniqion(ep, nav->ion_bdsk9);
	}

    /* set time span for progress display */
    if (ts.time==0||te.time==0) {
        for (i=0;   i<obs->n;i++) if (obs->data[i].rcv==1) break;
        for (j=obs->n-1;j>=0;j--) if (obs->data[j].rcv==1) break;
        if (i<j) {
            if (ts.time==0) ts=obs->data[i].time;
            if (te.time==0) te=obs->data[j].time;
            settspan(ts,te);
        }
    }
    return 1;
}
/* free obs and nav data -----------------------------------------------------*/
static void freeobsnav(obs_t *obs, nav_t *nav)
{
    trace(3,"freeobsnav:\n");
    
    free(obs->data); obs->data=NULL; obs->n =obs->nmax =0;
    free(nav->eph ); nav->eph =NULL; nav->n =nav->nmax =0;
    free(nav->geph); nav->geph=NULL; nav->ng=nav->ngmax=0;
    free(nav->seph); nav->seph=NULL; nav->ns=nav->nsmax=0;
}
/* average of single position ------------------------------------------------*/
static int avepos(double *ra, int rcv, const obs_t *obs, const nav_t *nav,
                  const prcopt_t *opt)
{
    obsd_t data[MAXOBS];
    gtime_t ts={0};
    sol_t sol={{0}};
    int i,j,n=0,m,iobs;
    char msg[128];
    
    trace(3,"avepos: rcv=%d obs.n=%d\n",rcv,obs->n);
    
    for (i=0;i<3;i++) ra[i]=0.0;
    
    for (iobs=0;(m=nextobsf(obs,&iobs,rcv))>0;iobs+=m) {
        
        for (i=j=0;i<m&&i<MAXOBS;i++) {
            data[j]=obs->data[iobs+i];
            if ((satsys(data[j].sat,NULL)&opt->navsys)&&
                opt->exsats[data[j].sat-1]!=1) j++;
        }
        if (j<=0||!screent(data[0].time,ts,ts,1.0)) continue; /* only 1 hz */
        
        if (!pntpos(data,j,nav,opt,&sol,NULL,NULL,msg)) continue;
        
        for (i=0;i<3;i++) ra[i]+=sol.rr[i];
        n++;
    }
    if (n<=0) {
        trace(1,"no average of base station position\n");
        return 0;
    }
    for (i=0;i<3;i++) ra[i]/=n;
    return 1;
}
/* station position from file ------------------------------------------------*/
static int getstapos(const char *file, char *name, double *r)
{
    FILE *fp;
    char buff[256],sname[256],*p,*q;
    double pos[3];
    
    trace(3,"getstapos: file=%s name=%s\n",file,name);
    
    if (!(fp=fopen(file,"r"))) {
        trace(1,"station position file open error: %s\n",file);
        return 0;
    }
    while (fgets(buff,sizeof(buff),fp)) {
        if ((p=strchr(buff,'%'))) *p='\0';
        
        if (sscanf(buff,"%lf %lf %lf %s",pos,pos+1,pos+2,sname)<4) continue;
        
        for (p=sname,q=name;*p&&*q;p++,q++) {
            if (toupper((int)*p)!=toupper((int)*q)) break;
        }
        if (!*p) {
            pos[0]*=D2R;
            pos[1]*=D2R;
            pos2ecef(pos,r);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    trace(1,"no station position: %s %s\n",name,file);
    return 0;
}
/* antenna phase center position ---------------------------------------------*/
static int antpos(prcopt_t *opt, int rcvno, const obs_t *obs, const nav_t *nav,
                  const sta_t *sta, const char *posfile)
{
    double *rr=rcvno==1?opt->ru:opt->rb,del[3],pos[3],dr[3]={0};
    int i,postype=rcvno==1?opt->rovpos:opt->refpos;
    char *name;
    
    trace(3,"antpos  : rcvno=%d\n",rcvno);
    
    if (postype==POSOPT_SINGLE) { /* average of single position */
        if (!avepos(rr,rcvno,obs,nav,opt)) {
            //showmsg("error : station pos computation");
			printf("error : station pos computation\n");
            return 0;
        }
    }
    else if (postype==POSOPT_FILE) { /* read from position file */
        name=stas[rcvno==1?0:1].name;
        if (!getstapos(posfile,name,rr)) {
            showmsg("error : no position of %s in %s",name,posfile);
            return 0;
        }
    }
    else if (postype==POSOPT_RINEX) { /* get from rinex header */
        if (norm(stas[rcvno==1?0:1].pos,3)<=0.0) {
            showmsg("error : no position in rinex header");
            trace(1,"no position position in rinex header\n");
            return 0;
        }
        /* antenna delta */
        if (stas[rcvno==1?0:1].deltype==0) { /* enu */
            for (i=0;i<3;i++) del[i]=stas[rcvno==1?0:1].del[i];
            del[2]+=stas[rcvno==1?0:1].hgt;
            ecef2pos(stas[rcvno==1?0:1].pos,pos);
            enu2ecef(pos,del,dr);
        }
        else { /* xyz */
            for (i=0;i<3;i++) dr[i]=stas[rcvno==1?0:1].del[i];
        }
        for (i=0;i<3;i++) rr[i]=stas[rcvno==1?0:1].pos[i]+dr[i];
    }
    return 1;
}
/* open processing session ----------------------------------------------------
    pcvs 是指卫星天线信息
    pcvr 是指接收机天线信息 */
static int openses(const prcopt_t *popt, const solopt_t *sopt,
                   const filopt_t *fopt, nav_t *nav, pcvs_t *pcvs, pcvs_t *pcvr)
{
    int i;
    trace(3,"openses :\n");
    // 如果没有天线信息，且读取也读取不到，就报错（level-1）
    /* read satellite antenna parameters */
    if (*fopt->satantp&&!(readpcv(fopt->satantp,pcvs))) 
    {
        showmsg("error : no sat ant pcv in %s",fopt->satantp);
        trace(1,"sat antenna pcv read error: %s\n",fopt->satantp);
        return 0;
    }
    /* read receiver antenna parameters */
    if (*fopt->rcvantp&&!(readpcv(fopt->rcvantp,pcvr))) 
    {
        showmsg("error : no rec ant pcv in %s",fopt->rcvantp);
        trace(1,"rec antenna pcv read error: %s\n",fopt->rcvantp);
        return 0;
    }

    // 大地水准面模型
    /* open geoid data */
    if (sopt->geoid>0&&*fopt->geoid) {
        if (!opengeoid(sopt->geoid,fopt->geoid)) 
        {
            showmsg("error : no geoid data %s",fopt->geoid);
            trace(2,"no geoid data %s\n",fopt->geoid);
        }
    }


    /* use satellite L2 offset if L5 offset does not exists */
    for (i=0;i<pcvs->n;i++) {
        if (norm(pcvs->pcv[i].off[2],3)>0.0) continue;
        matcpy(pcvs->pcv[i].off[2],pcvs->pcv[i].off[1], 3,1);
        matcpy(pcvs->pcv[i].var[2],pcvs->pcv[i].var[1],19,1);
    }
    for (i=0;i<pcvr->n;i++) {
        if (norm(pcvr->pcv[i].off[2],3)>0.0) continue;
        matcpy(pcvr->pcv[i].off[2],pcvr->pcv[i].off[1], 3,1);
        matcpy(pcvr->pcv[i].var[2],pcvr->pcv[i].var[1],19,1);
    }
    return 1;
}
/* close procssing session ---------------------------------------------------*/
/*  */
static void closeses(nav_t *nav, pcvs_t *pcvs, pcvs_t *pcvr)
{
    trace(3,"closeses:\n");
    
    /* free antenna parameters */
    free(pcvs->pcv); pcvs->pcv=NULL; pcvs->n=pcvs->nmax=0;
    free(pcvr->pcv); pcvr->pcv=NULL; pcvr->n=pcvr->nmax=0;
    
    /* close geoid data */
    closegeoid();
    
    /* free erp data */
    free(nav->erp.data); nav->erp.data=NULL; nav->erp.n=nav->erp.nmax=0;
    
    /* close solution statistics and debug trace */
    rtkclosestat();
    traceclose();
}
/* set antenna parameters ----------------------------------------------------*/
static void setpcv(gtime_t time, prcopt_t *popt, nav_t *nav, const pcvs_t *pcvs,
                   const pcvs_t *pcvr, const sta_t *sta)
{
    pcv_t *pcv;
    double pos[3],del[3];
    int i,j,mode=PMODE_DGPS<=popt->mode&&popt->mode<=PMODE_FIXED;
    char id[64];
    
    /* set satellite antenna parameters */
    for (i=0;i<MAXSAT;i++) {
        if (!(satsys(i+1,NULL)&popt->navsys)) continue;
        if (!(pcv=searchpcv(i+1,"",time,pcvs))) {
            satno2id(i+1,id);
           // trace(3,"no satellite antenna pcv: %s\n",id);
            continue;
        }
        nav->pcvs[i]=*pcv;
    }
    for (i=0;i<(mode?2:1);i++) {
        if (!strcmp(popt->anttype[i],"*")) { /* set by station parameters */
            strcpy(popt->anttype[i],sta[i].antdes);
            if (sta[i].deltype==1) { /* xyz */
                if (norm(sta[i].pos,3)>0.0) {
                    ecef2pos(sta[i].pos,pos);
                    ecef2enu(pos,sta[i].del,del);
                    for (j=0;j<3;j++) popt->antdel[i][j]=del[j];
                }
            }
            else { /* enu */
                for (j=0;j<3;j++) popt->antdel[i][j]=stas[i].del[j];
            }
        }
        if (!(pcv=searchpcv(0,popt->anttype[i],time,pcvr))) {
            trace(2,"no receiver antenna pcv: %s\n",popt->anttype[i]);
            *popt->anttype[i]='\0';
            continue;
        }
        strcpy(popt->anttype[i],pcv->type);
        popt->pcvr[i]=*pcv;
    }
}
/* read ocean tide loading parameters ----------------------------------------*/
static void readotl(prcopt_t *popt, const char *file, const sta_t *sta)
{
    int i,mode=PMODE_DGPS<=popt->mode&&popt->mode<=PMODE_FIXED;
    
    for (i=0;i<(mode?2:1);i++) {
        readblq(file,sta[i].name,popt->odisp[i]);
    }
}
/* write header to output file -----------------------------------------------*/
static int outhead(const char *outfile, const char **infile, int n,
                   const prcopt_t *popt, const solopt_t *sopt)
{
    FILE *fp=stdout;
    
    trace(3,"outhead: outfile=%s n=%d\n",outfile,n);
    
    if (*outfile) {
        createdir(outfile);
        
        if (!(fp=fopen(outfile,"w"))) {
            showmsg("error : open output file %s",outfile);
            return 0;
        }
    }
    /* output header */
    outheader(fp,infile,n,popt,sopt);
    
    if (*outfile) fclose(fp);
    
    return 1;
}
/* open output file for append -----------------------------------------------*/
static FILE *openfile(const char *outfile)
{
    trace(3,"openfile: outfile=%s\n",outfile);
    
    return !*outfile?stdout:fopen(outfile,"a");
}
/* execute processing session ------------------------------------------------*/
//处理进程会话
static int execses(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                   const solopt_t *sopt, const filopt_t *fopt, int flag,
    const char **infile, const int *index, int n, const char *outfile)
{
	FILE *fp, *fpres, *fpout, *fdop;
    prcopt_t popt_=*popt;
	char tracefile[1024], statfile[1024], path[1024], *ext, filestr[1024],dopdatafile[1024], figurefile[1024];
	char cprn[10];
	char timestr[128];

	double epoch[6], pos[3],rr[3],r,e[3];
	double *rs, *dts, *var;
	int i, j, sys, nobs,ns, prn, svh[MAXOBS];
	obsd_t obs[MAXOBS * 2]; /* for rover and base */
	gtime_t teph;
	double azels[MAXOBS * 2] = {0.0}, dop[4];


    trace(3,"execses : n=%d outfile=%s\n",n,outfile);
    
    /* open debug trace */
    if (flag&&sopt->trace>=0) {
        if (*outfile) {
            strcpy(tracefile,outfile);
            strcat(tracefile,".trace");
        }
        else {
            strcpy(tracefile,fopt->trace);
        }
        traceclose();
        traceopen(tracefile);
        tracelevel(sopt->trace);
    }
    /* read ionosphere data file */
	if (*fopt->iono && (ext = (char*)strrchr(fopt->iono, '.'))) 
    {
        if (strlen(ext)==4&&(ext[3]=='i'||ext[3]=='I')) 
        {
            reppath(fopt->iono,path,ts,"","");
            readtec(path,&navs,1);
        }
    }
    /* read erp data */
    if (*fopt->eop) {
        free(navs.erp.data); navs.erp.data=NULL; navs.erp.n=navs.erp.nmax=0;
        reppath(fopt->eop,path,ts,"","");
        if (!readerp(path,&navs.erp)) {
            showmsg("error : no erp data %s",path);
            trace(2,"no erp data %s\n",path);
        }
    }
    /* read obs and nav data */
    //读取观测值和星历
	printf("processing : reading data... \n");
	prgbar = 0;
    if (!readobsnav(ts,te,ti,infile,index,n,&popt_,&obss,&navs,stas)) return 0;
    
    /* read dcb parameters */
    if (*fopt->dcb) {
        reppath(fopt->dcb,path,ts,"","");
        readdcb(path,&navs,stas);
    }
    /* set antenna paramters */
    if (popt_.mode!=PMODE_SINGLE) {
        setpcv(obss.n>0?obss.data[0].time:timeget(),&popt_,&navs,&pcvss,&pcvsr,
               stas);
    }
    /* read ocean tide loading parameters */
    if (popt_.mode>PMODE_SINGLE&&*fopt->blq) {
        readotl(&popt_,fopt->blq,stas);
    }
    /* rover/reference fixed position */
    if (popt_.mode==PMODE_FIXED) {
        if (!antpos(&popt_,1,&obss,&navs,stas,fopt->stapos)) {
            freeobsnav(&obss,&navs);
            return 0;
        }
    }
    else if (PMODE_DGPS<=popt_.mode&&popt_.mode<=PMODE_STATIC) {
        if (!antpos(&popt_,2,&obss,&navs,stas,fopt->stapos)) {
            freeobsnav(&obss,&navs);
            return 0;
        }
    }
    /* open solution statistics */
    if (flag&&sopt->sstat>0) 
    {
        strcpy(statfile,outfile);
        strcat(statfile,".stat");
        rtkclosestat();
        rtkopenstat(statfile,sopt->sstat);
    }
    /* write header to output file */
    if (flag&&!outhead(outfile,infile,n,&popt_,sopt)) {
        freeobsnav(&obss,&navs);
        return 0;
    }
    iobsu=iobsr=isbs=ilex=revs=aborts=0;
	strcpy(outsppfile, outfile);
	/* sat position only */
	if (popt_.outsat != 0)
	{
		strcpy(filestr, outsppfile);
		fpout = fopen(filestr, "w");
		fclose(fpout);
		strcpy(filestr, outsppfile);
		fpout = fopen(strcat(filestr, "psu_snr"), "w");
		fclose(fpout);


		strcpy(filestr, outsppfile);
		strcpy(dopdatafile, outsppfile);
		strcpy(figurefile, outsppfile);
		strcat(dopdatafile, "psu_dop");
		strcat(figurefile, "psu_dop.png");
		fdop = fopen(strcat(filestr, "psu_dop"), "w");


		strcpy(filestr, outsppfile);
		fpres = fopen(strcat(filestr, "psu_res"), "w");
		if (timediff(te, ts) == 0.0){ teph = timeget();}
		else{ teph = te; }

		gtime_t tss, tee;
		//double ep[6] = { 2020, 6, 1, 0, 0, 0};
		//tss = epoch2time(ep);
		//tee = timeadd(tss, 86400);
		tss = te;
		tee = timeadd(tss, 0);
		teph = tss;

		fprintf(fpres, "%23s", "%BDT_sat        Obs_Time");
//		for (teph = tss; timediff(teph,tee)<0.0; teph=timeadd(teph, 30.0)){
			for (i = 0, nobs = 0; i < MAXSAT; i++)
			{
				if (!(sys = satsys(i + 1, &prn))) continue;
				/* excluded satellite? */
				if (satexclude(i + 1, 0.0, 0, &popt_))continue;
				satno2id(i + 1, cprn);
				fprintf(fpres, "%21s ", cprn);
				obs[nobs].P[0] = 1E-3;
				obs[nobs].time = teph;
				obs[nobs].sat = i + 1;
				nobs++;
			}
			fprintf(fpres, "\n");
			rs = mat(6, nobs); dts = mat(2, nobs); var = mat(1, nobs);
			satposs(teph, obs, nobs, &navs, &popt_,popt_.sateph, rs, dts, var, svh);

			time2str(teph, timestr, 3);
			fprintf(fpres, "%23s ", timestr);
			for (i = 0; i < nobs; i++)
			{
				pos[0] = pos[1] = pos[2] = 0.0;
				if (norm(rs + i * 6, 3)>0.0){
					ecef2pos(rs + i * 6, pos);
					for (j = 0; j < 2; j++)pos[j] = pos[j] * R2D;
					if (fabs(pos[0]) > 90.0 || pos[2] <= 0.0) { pos[0] = pos[1] = pos[2] = 0.0; }
					sys = satsys(obs[i].sat, &prn);

					//printf("sat= %d %14.3f %14.3f %14.3f %14.7f %14.7f %14.3f\n", prn, rs[i * 6], rs[i * 6 + 1], rs[i * 6 + 2], pos[0], pos[1], pos[2]);
					if (pos[1] < 0)pos[1] += 360.0;
					if (pos[0] < 0)pos[0] += 180.0;
					
				}
				fprintf(fpres, "%7.7d%7.7d%7.7d ", (int)(pos[0] * 1E4), (int)(pos[1] * 1E4), (int)(pos[2]/10.0));

			}
			fprintf(fpres, "\n");
//		}

	/* PDOP */
		int minlat = -90, maxlat = 90, minlon = 0,maxlon = 360;
		int dlat = 5, dlon = 5;
		
		for (int ilat = minlat; ilat <= maxlat; ilat += dlat){
			for (int ilon = minlon; ilon <= maxlon; ilon += dlon){
				/* user position */
				pos[0] = ilat*D2R; pos[1] = ilon*D2R; pos[2] = 0.0;
				pos2ecef(pos, rr);
				for (i = 0; i < nobs; i++){
					if (norm(rs + i * 6, 3) <= 0.0)continue;
					/* geometric distance/azimuth/elevation angle */
					if ((r = geodist(sys, rs + i * 6, rr, e, NULL)) <= 0.0 ||satazel(pos, e, azels + i * 2) < popt->elmin)continue;
				}
				/* large gdop check */
				for (i = ns = 0; i<nobs; i++) {
					if (azels[1 + i * 2] < popt->elmin) continue;
					azels[ns * 2] = azels[i * 2];
					azels[1 + ns * 2] = azels[1 + i * 2];
					ns++;
				}
				dops(ns, azels, popt->elmin, dop);
				int dlat = ilat;
				int dlon = ilon;// <= 180 ? ilon : ilon - 360;
				fprintf(fdop, "%6d %6d %14.3f\n", dlat, dlon, dop[1]);
			}
			
		}
		fclose(fdop);

		/* produce png picture using pdop data */
		char cmd_str[2048];
		double ep[6];
		int sys;
		time2epoch(ts, ep);
		if(popt_.navsys==1) sys=2;
		else if(popt_.navsys==8) sys=3;
		else if(popt_.navsys==4) sys=4;
		else sys=1;
		/* datafile , figurename, maxcolor and mincolor*/
		//sprintf(cmd_str, "map_dop %s %s %f %f", dopdatafile, figurefile, 3.0, 0.5);
        sprintf(cmd_str, "%s %s %s %1d %02.0lf:%02.0lf %04.0lf-%02.0lf-%02.0lf",
            "start /wait /min E:/pvt/zhghu_bin/gnssmap.exe", dopdatafile, figurefile, sys, ep[3], ep[4], ep[0], ep[1], ep[2]);
        system(cmd_str);

		fclose(fpres);
		fclose(fdop);
		free(rs); free(dts); free(var);
		return 1;
	}

    if (popt_.mode==PMODE_SINGLE||popt_.soltype==0) {
        if ((fp=openfile(outfile))) {
            procpos(fp,&popt_,sopt,0); /* forward */
            fclose(fp);
        }
    }
    else if (popt_.soltype==1) {
        if ((fp=openfile(outfile))) {
            revs=1; iobsu=iobsr=obss.n-1; isbs=sbss.n-1; ilex=lexs.n-1;
            procpos(fp,&popt_,sopt,0); /* backward */
            fclose(fp);
        }
    }
    else { /* combined */
        solf=(sol_t *)malloc(sizeof(sol_t)*nepoch);
        solb=(sol_t *)malloc(sizeof(sol_t)*nepoch);
        rbf=(double *)malloc(sizeof(double)*nepoch*3);
        rbb=(double *)malloc(sizeof(double)*nepoch*3);
        
        if (solf&&solb) {
            isolf=isolb=0;
            procpos(NULL,&popt_,sopt,1); /* forward */
            revs=1; iobsu=iobsr=obss.n-1; isbs=sbss.n-1; ilex=lexs.n-1;
            procpos(NULL,&popt_,sopt,1); /* backward */
            
            /* combine forward/backward solutions */
            if (!aborts&&(fp=openfile(outfile))) {
                combres(fp,&popt_,sopt);
                fclose(fp);
            }
        }
        else showmsg("error : memory allocation");
        free(solf);
        free(solb);
        free(rbf);
        free(rbb);
    }
    /* free obs and nav data */
    freeobsnav(&obss,&navs);
    
    return aborts?1:0;
}
/* execute processing session for each rover ---------------------------------
* 为每个流动站执行处理会话，功能逻辑见execses_b
*/
static int execses_r(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                     const solopt_t *sopt, const filopt_t *fopt, int flag,
                     const char **infile, const int *index, int n, const char *outfile,
                     const char *rov)
{
    gtime_t t0={0};
    int i,stat=0;
    char* ifile[MAXINFILE];
    char* ofile = (char*)malloc(1024);
    char* rov_, * p, * q, s[64] = "";
    const char* local_ifile[MAXINFILE], * local_ofile;
    trace(3,"execses_r: n=%d outfile=%s\n",n,outfile);
    
    for (i=0;i<n;i++) if (strstr(infile[i],"%r")) break;
    
    if (i<n) { /* include rover keywords */
        if (!(rov_=(char *)malloc(strlen(rov)+1))) return 0;
        strcpy(rov_,rov);
        
        for (i=0;i<n;i++) {
            if (!(ifile[i]=(char *)malloc(1024))) {
                free(rov_); for (;i>=0;i--) free(ifile[i]);
                return 0;
            }
        }
        for (p=rov_;;p=q+1) { /* for each rover */
            if ((q=strchr(p,' '))) *q='\0';
            
            if (*p) {
                strcpy(proc_rov,p);
                if (ts.time) time2str(ts,s,0); else *s='\0';
                if (checkbrk("reading    : %s",s)) {
                    stat=1;
                    break;
                }
                for (i=0;i<n;i++) reppath(infile[i],ifile[i],t0,p,"");
                reppath(outfile,ofile,t0,p,"");
                
                /* execute processing session */
                for (int i = 0; i < MAXINFILE; i++)local_ifile[i] = ifile[i];
                local_ofile = ofile;
                stat=execses(ts,te,ti,popt,sopt,fopt,flag, local_ifile,index,n, local_ofile);
            }
            if (stat==1||!q) break;
        }
        free(rov_); for (i=0;i<n;i++) free(ifile[i]);
    }
    else {
        /* execute processing session */
        stat=execses(ts,te,ti,popt,sopt,fopt,flag,infile,index,n,outfile);
    }
    return stat;
}
/* execute processing session for each base station --------------------------*/
static int execses_b(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                     const solopt_t *sopt, const filopt_t *fopt, int flag,
    const char **infile, const int *index, int n, const char *outfile,
                     const char *rov, const char *base)
{
    gtime_t t0={0};
    int i,stat=0;
    char *ifile[MAXINFILE],ofile[1024],*base_,*p,*q,s[64];
    const char* local_ifile[MAXINFILE], * local_ofile;
    
    trace(3,"execses_b: n=%d outfile=%s\n",n,outfile);
    
    /* read prec ephemeris and sbas data */
   // readpreceph(infile,n,popt,&navs,&sbss,&lexs);
   // 
    // 若文件地址名中存在“%b”，则break
    for (i=0;i<n;i++) if (strstr(infile[i],"%b")) break;
    
    //若上面的函数break了，i会小于n，触发下面的函数内容
    //功能是替换基准站&流动站文件路径表达
    if (i<n) { /* include base station keywords */
        if (!(base_=(char *)malloc(strlen(base)+1))) 
        {
            //检查内存是否不够，如果不够，就清理一些导航文件的内存
            freepreceph(&navs,&sbss,&lexs);
            return 0;
        }
        strcpy(base_,base);
        
        for (i=0;i<n;i++) {
            if (!(ifile[i]=(char *)malloc(1024)))
            {
                //检查内存是否不够，如果不够，就清理一些导航文件的内存，并把先前分配的base_ & ifile[]文件内存清空
                free(base_); for (;i>=0;i--) free(ifile[i]);
                freepreceph(&navs,&sbss,&lexs);
                return 0;
            }
        }
        for (p=base_;;p=q+1) { /* for each base station */
            //将p字符串从第一个‘ ’处截断
            if ((q=strchr(p,' '))) *q='\0';
            
            if (*p) 
            {
                //如果*p（也即base_不为空，则执行以下内容）
                strcpy(proc_base,p);
                //如果存在ts.time（即有观测值），则将其转为string并存入s，否则s为空
                if (ts.time) time2str(ts,s,0); else *s='\0';
                if (checkbrk("reading    : %s",s)) //如果s不为空
                {
                    stat=1;
                    break;
                }
                for (i=0;i<n;i++) reppath(infile[i],ifile[i],t0,"",p);  //替换文件路径表达
                reppath(outfile,ofile,t0,"",p);
                for (int i = 0; i < MAXINFILE; i++)local_ifile[i] = ifile[i];
                local_ofile = ofile;
                stat = execses_r(ts, te, ti, popt, sopt, fopt, flag, local_ifile, index, n, local_ofile, rov);
            }
            if (stat==1||!q) break;
        }
        free(base_); for (i=0;i<n;i++) free(ifile[i]);
    }
    else {
        stat=execses_r(ts,te,ti,popt,sopt,fopt,flag,infile,index,n,outfile,rov);
    }
    /* free prec ephemeris and sbas data */
    freepreceph(&navs,&sbss,&lexs);
    
    return stat;
}
/* post-processing positioning -------------------------------------------------
* post-processing positioning
* args   : gtime_t ts       I   processing start time (ts.time==0: no limit)
*        : gtime_t te       I   processing end time   (te.time==0: no limit)
*          double ti        I   processing interval  /处理时间间隔 (s) (0:all)
*          double tu        I   processing unit time /处理单位时间 (s) (0:all)
*          prcopt_t *popt   I   processing options
*          solopt_t *sopt   I   solution options
*          filopt_t *fopt   I   file options
*          char   **infile  I   input files (see below)
*          int    n         I   number of input files   输入文件个数
*          char   *outfile  I   output file ("":stdout, see below)
*          char   *rov      I   rover id list        (separated by " ")
*          char   *base     I   base station id list (separated by " ")
* return : status (0:ok,0>:error,1:aborted)
* notes  : input files should contain observation data, navigation data, precise 
*          ephemeris/clock (optional), sbas log file (optional), ssr message
*          log file (optional) and tec grid file (optional). only the first 
*          observation data file in the input files is recognized as the rover
*          data.
*
*          the type of an input file is recognized by the file extention as ]
*          follows:
*              .sp3,.SP3,.eph*,.EPH*: precise ephemeris (sp3c)
*              .sbs,.SBS,.ems,.EMS  : sbas message log files (rtklib or ems)
*              .lex,.LEX            : qzss lex message log files
*              .rtcm3,.RTCM3        : ssr message log files (rtcm3)
*              .*i,.*I              : tec grid files (ionex)
*              .fcb,.FCB            : satellite fcb
*              others               : rinex obs, nav, gnav, hnav, qnav or clock
*
*          inputs files can include wild-cards (*). if an file includes
*          wild-cards, the wild-card expanded multiple files are used.
*
*          inputs files can include keywords. if an file includes keywords,
*          the keywords are replaced by date, time, rover id and base station
*          id and multiple session analyses run. refer reppath() for the
*          keywords.
*
*          the output file can also include keywords. if the output file does
*          not include keywords. the results of all multiple session analyses
*          are output to a single output file.
*
*          ssr corrections are valid only for forward estimation.
*-----------------------------------------------------------------------------*/
/*  关于**infile的逻辑
*   该程序使用char *infile[16]，来存储输入文件地址，每一个infile[x]都代表一个文件地址的指针
*   *infile[x]代表文件地址，是一串字符，例如“D:\AAA-Study\SPP-PPP\LXZ-rtklib\data\AC230650.25O”
*   **infile[x]代表该地址下的文件，在实例中就为O文件
*/
extern int postpos(gtime_t ts, gtime_t te, double ti, double tu,
    const prcopt_t* popt, const solopt_t* sopt,
    const filopt_t* fopt, const char** infile, int n, const char* outfile,
    const char* rov, const char* base)
{
    gtime_t tts = { -1 }, tte = { -1 }, ttte = { -1 };
    double tunit, tss;
    int i, j, k, nf = -1, stat = 0, week, flag = 1, index[MAXINFILE] = { 0 };
    char* ifile[MAXINFILE], ofile[1024] = "";
    const char* local_ifile[MAXINFILE], * local_ofile;
    const char* ext;

    trace(3, "postpos : ti=%.0f tu=%.0f n=%d outfile=%s\n", ti, tu, n, outfile);

    /* open processing session */
    //读取天线等信息，如果没有天线信息就是返回0
    if (!openses(popt, sopt, fopt, &navs, &pcvss, &pcvsr)) return -1;

    //如果开始与结束时间存在，并且单位时间存在，进行以下
    if (ts.time != 0 && te.time != 0 && tu >= 0.0)
    {
        if (timediff(te, ts) < 0.0)
        {
            showmsg("error : no period");
            closeses(&navs, &pcvss, &pcvsr);  //关闭对话释放内存
            return 0;
        }
        for (i = 0; i < MAXINFILE; i++)
        {
            if (!(ifile[i] = (char*)malloc(1024))) {
                for (; i >= 0; i--) free(ifile[i]);
                closeses(&navs, &pcvss, &pcvsr);
                return -1;
            }
        }
        if (tu == 0.0 || tu > 86400.0 * MAXPRCDAYS) tu = 86400.0 * MAXPRCDAYS;  //如果处理单位时间为零或过大，进行初始化限制
        settspan(ts, te);
        tunit = tu < 86400.0 ? tu : 86400.0;    //如果 tu 小于 86400.0，则 tunit = tu，否则 tunit = 86400.0。
        tss = tunit * (int)floor(time2gpst(ts, &week) / tunit);     //tss = tunit * （ts_GPS周内秒/tunit，再向下取整）

        for (i = 0;; i++) 
        { /* for each periods */
            tts = gpst2time(week, tss + i * tu);
            tte = timeadd(tts, tu - DTTOL);  //
            if (timediff(tts, te) > 0.0) break;
            if (timediff(tts, ts) < 0.0) tts = ts;
            if (timediff(tte, te) > 0.0) tte = te;

            strcpy(proc_rov, "");
            strcpy(proc_base, "");
            if (checkbrk("reading    : %s", time_str(tts, 0))) {
                stat = 1;
                break;
            }
            for (j = k = nf = 0; j < n; j++) {

                ext = strrchr(infile[j], '.');

                if (ext && (!strcmp(ext, ".rtcm3") || !strcmp(ext, ".RTCM3"))) {
                    strcpy(ifile[nf++], infile[j]);
                }
                else {
                    /* include next day precise ephemeris or rinex brdc nav */
                    ttte = tte;
                    if (ext && (!strcmp(ext, ".sp3") || !strcmp(ext, ".SP3") ||
                        !strcmp(ext, ".eph") || !strcmp(ext, ".EPH"))) {
                        ttte = timeadd(ttte, 3600.0);
                    }
                    else if (strstr(infile[j], "brdc")) {
                        ttte = timeadd(ttte, 7200.0);
                    }
                    nf += reppaths(infile[j], ifile + nf, MAXINFILE - nf, tts, ttte, "", "");
                }
                while (k < nf) index[k++] = j;

                if (nf >= MAXINFILE) {
                    trace(2, "too many input files. trancated\n");
                    break;
                }
            }
            if (!reppath(outfile, ofile, tts, "", "") && i > 0) flag = 0;

            /* execute processing session */
            for (int i = 0; i < MAXINFILE; i++)local_ifile[i] = ifile[i];
            local_ofile = ofile;
            stat = execses_b(tts, tte, ti, popt, sopt, fopt, flag, local_ifile, index, nf, local_ofile,
                rov, base);

            if (stat == 1) break;
        }
        for (i = 0; i < MAXINFILE; i++) free(ifile[i]);
    }
    else if (ts.time != 0) {
        for (i = 0; i < n && i < MAXINFILE; i++) {
            if (!(ifile[i] = (char*)malloc(1024))) {
                for (; i >= 0; i--) free(ifile[i]);
                return -1;
            }
            reppath(infile[i], ifile[i], ts, "", "");
            index[i] = i;
        }
        reppath(outfile, ofile, ts, "", "");

        /* execute processing session */
        for (int i = 0; i < MAXINFILE; i++)local_ifile[i] = ifile[i];
        local_ofile = ofile;
        stat = execses_b(tts, tte, ti, popt, sopt, fopt, flag, local_ifile, index, nf, local_ofile,
            rov, base);

        for (i = 0; i < n && i < MAXINFILE; i++) free(ifile[i]);
    }
    else {
        for (i = 0; i < n; i++) index[i] = i;

        /* execute processing session */
        if (popt->mode == PMODE_SINGLE)
            stat = execses(ts, te, ti, popt, sopt, fopt, 1, infile, index, n, outfile);
        else
            stat = execses_b(ts, te, ti, popt, sopt, fopt, 1, infile, index, n, outfile, rov, base);
    }
    /* close processing session */
    closeses(&navs, &pcvss, &pcvsr);
    return stat;
}
