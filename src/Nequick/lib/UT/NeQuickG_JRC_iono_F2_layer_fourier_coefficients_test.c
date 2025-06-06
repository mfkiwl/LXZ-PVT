/**
 * NeQuickG F2 layer Unit test
 *
 * @author Angela Aragon-Angel (maria-angeles.aragon@ec.europa.eu)
 * @ingroup NeQuickG_JRC_UT
 * @copyright Joint Research Centre (JRC), 2019<br>
 *  This software has been released as free and open source software
 *  under the terms of the European Union Public Licence (EUPL), version 1.<br>
 *  Questions? Submit your query at https://www.gsc-europa.eu/contact-us/helpdesk
 * @file
 */
#include "NeQuickG_JRC_iono_F2_layer_fourier_coefficients_test.h"

#ifdef FTR_MODIP_CCIR_AS_CONSTANTS
#include "NeQuickG_JRC_CCIR.h"
#endif
#include "NeQuickG_JRC_iono_F2_layer_fourier_coefficients.h"
#include "NeQuickG_JRC_macros.h"

#define ITU_F2_LAYER_INTERPOLATED_COEFF_TEST_VECTORS_COUNT (1)
#define ITU_F2_LAYER_INTERPOLATED_COEFF_FOURIER_TEST_VECTORS_COUNT (2)
#define ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON (1e-5)

typedef struct ITU_F2_layer_coefficients_test_st {
  double_t Azr;
  F2_coefficient_array_t AF2;
  Fm3_coefficient_array_t Am3;
} ITU_F2_layer_coefficients_test_t;

typedef struct ITU_F2_layer_coefficients_fourier_test_st {
  double_t Azr;
  NeQuickG_time_t time;
  F2_fourier_coefficient_array_t CF2;
  Fm3_fourier_coefficient_array_t Cm3;
} ITU_F2_layer_coefficients_fourier_test_t;

static const ITU_F2_layer_coefficients_test_t
  ITU_F2_layer_coefficients_test_vector[ITU_F2_LAYER_INTERPOLATED_COEFF_TEST_VECTORS_COUNT] =
{
  {
    186.328060,
    {
      { 12.605340, 0.313256, 0.249495, 0.049053, 0.029104, 0.007502, -0.088684, 0.018222, -0.001853, 0.046916, 0.010309, 0.011487, -0.011607},
      { 1.169928, 0.112289, -0.150386, -0.333868, 0.604549, 0.670627, -0.565507, 0.281560, -0.094419, -0.069452, 0.031858, 0.046254, 0.045217},
      { 48.033505, -5.233178, -3.254403, 4.474504, 2.049914, -0.155251, 0.129335, 0.234437, 0.094904, -1.970059, -0.749943, 0.394905, 0.035730},
      { -33.384710, 3.235633, 10.083203, 4.355962, -8.718357, -7.505598, 7.846329, -2.907102, 1.893942, -0.889924, -0.812209, -0.289308, 0.027877},
      { -232.876672, 34.143300, 3.098905, -49.103239, -17.359770, 2.064551, 3.573960, 2.567447, -0.274293, 9.659007, 5.943820, -4.429119, 1.213721},
      { 215.714239, -43.861678, -21.895338, -9.012181, 57.491373, 22.591265, -39.741690, 16.885715, -13.466449, 8.860139, 7.624284, 2.393455, -1.665282},
      { 418.308887, -76.120002, 15.002122, 158.305733, 49.535624, -4.148387, -14.452858, -14.271928, -0.077570, -16.268280, -16.856444, 13.790904, -4.336880},
      { -512.396227, 121.372980, -39.916676, -6.157677, -145.930864, -27.028738, 85.604631, -44.157515, 35.440471, -23.537707, -22.235131, -7.450479, 4.319500},
      { -361.528414, 70.027646, -25.313509, -195.287031, -58.597947, 2.302665, 18.648192, 20.950511, 0.341560, 10.592821, 19.528544, -16.675782, 4.710296},
      { 513.824929, -131.762305, 114.833015, 25.195737, 155.836873, 12.221157, -82.395676, 48.848563, -38.104631, 24.958091, 25.420673, 8.674223, -3.799770},
      { 122.115232, -23.344245, 10.254202, 81.816004, 24.323755, -0.056900, -7.803140, -9.521312, -0.044382, -2.056354, -7.867321, 6.918368, -1.565617},
      { -184.986330, 51.275826, -63.012360, -14.008187, -59.320434, -0.970336, 29.374678, -18.937461, 14.300356, -9.301002, -10.049047, -3.369804, 1.071633},
      { -0.045080, 1.463798, 1.400662, 0.097343, -0.016736, -0.014765, -0.002148, 0.015800, -0.011023, 0.013787, 0.029012, -0.007102, -0.014518},
      { 0.436505, -1.508201, 1.166348, -0.131102, -0.011230, -0.023335, -0.001966, -0.057908, -0.005647, -0.025366, 0.017572, 0.002181, -0.035899},
      { 1.554252, -1.075776, -0.789063, -0.594607, 0.319782, -0.291296, 0.284122, -0.192956, 0.002946, -0.114094, 0.344783, -0.124144, 0.051071},
      { 1.855770, -1.139613, 0.532812, -0.173064, -0.052377, 1.094478, 0.544593, 0.337293, -0.461860, -0.196080, 0.022977, -0.045425, -0.671778},
      { -1.968465, 14.522424, -25.997919, -1.194876, -0.387456, -2.994397, 0.029345, 0.564826, 1.660505, -1.351524, -0.527586, 0.186069, 0.062935},
      { -11.835563, 27.618008, 25.481320, 6.775228, 1.698771, 1.559801, -1.900554, 2.240655, -0.783190, -1.149844, -0.204631, 0.390659, 1.622043},
      { -17.403655, 12.987714, 4.412895, -4.984779, -12.556048, 3.609189, -7.054924, 3.897800, 5.543111, -1.773318, -4.057586, 4.199129, -0.946793},
      { -32.927709, 27.954948, -21.268468, 12.495880, -2.042190, -4.871082, -4.102766, -9.817850, 11.511584, -0.523183, -2.707436, 0.307591, 11.078793},
      { 22.158435, 62.046546, 147.999461, -12.060964, 20.119652, 28.446452, -3.505217, -1.059723, -11.558060, 8.072343, 1.949282, 2.143111, -2.159692},
      { 96.541401, -187.242190, -13.091500, -52.208576, -7.885607, -11.490833, 20.957980, -16.102596, 5.436303, 10.621450, 3.901198, -4.562195, -13.612887},
      { 48.808955, -90.991886, -32.387570, 73.855967, 73.826064, -24.493968, 39.706659, -20.616878, -40.576198, 16.695069, 12.193076, -25.751294, 12.374516},
      { 237.029518, -119.498973, 123.728080, -77.027466, 4.858599, 7.609107, 6.970890, 64.420970, -68.897238, 6.634095, 20.252329, -2.657578, -54.528912},
      { -88.522241, -464.750840, -234.627663, 78.807865, -80.213575, -86.361789, 22.886626, -3.373235, 23.292733, -17.649945, -5.732901, -9.734850, 7.778405},
      { -292.668794, 395.857964, -303.958173, 154.059584, 5.342703, 26.919782, -69.210193, 39.989035, -12.651589, -31.052031, -15.532030, 12.890232, 40.724752},
      { -20.395999, 235.606784, 94.500119, -238.540161, -163.269300, 75.390470, -91.505977, 38.403044, 107.310254, -48.038737, -7.519126, 54.907698, -49.662874},
      { -684.433624, 224.912696, -263.724829, 169.772776, 15.997527, -7.424005, 7.177101, -154.977230, 164.445113, -5.829790, -54.717991, 10.806883, 115.870440},
      { 133.375841, 782.519958, 166.274593, -133.409228, 103.859156, 107.445607, -42.444629, 9.380890, -14.072954, 16.167417, 10.017364, 12.182878, -9.882188},
      { 382.339775, -392.021838, 660.500025, -200.744825, 12.940374, -23.967628, 90.328150, -42.402037, 14.679182, 38.083262, 21.406621, -12.752926, -50.425665},
      { -57.614769, -251.378380, -151.798704, 291.068092, 159.085047, -102.635903, 94.627585, -26.977914, -119.712376, 56.337847, -10.079401, -46.229550, 73.995106},
      { 839.338599, -173.226401, 215.648889, -146.987954, -48.045354, 12.826667, -31.528798, 156.099058, -173.588504, -12.062153, 62.759427, -15.373148, -111.019926},
      { -66.847508, -400.030211, -52.434073, 70.643090, -42.142479, -47.543178, 23.876324, -5.754827, 0.211416, -5.117637, -6.111011, -4.510637, 4.349204},
      { -177.191566, 155.414498, -375.407702, 94.949884, -10.775809, 6.727726, -41.683171, 16.639216, -6.969563, -16.785451, -9.724054, 3.801352, 22.168354},
      { 48.242564, 90.620046, 87.325395, -123.816045, -58.131081, 50.128683, -36.277517, 5.098232, 47.392135, -22.992987, 9.356549, 12.630013, -36.661218},
      { -363.282598, 42.633189, -53.050249, 39.692389, 27.371883, -10.584300, 22.807966, -56.583317, 67.434613, 12.793143, -25.901444, 7.021491, 39.329433},
      { 0.038098, -0.543312, 0.262068, -0.687094, 1.198206, -0.057017, 0.092313, 0.043021, -0.024797, -0.032974, 0.031728, -0.012591, -0.011246},
      { 0.120357, -0.375800, -0.159186, -1.230970, -0.878177, -0.064158, -0.032577, 0.006907, -0.018462, -0.045782, 0.013923, -0.000234, -0.007239},
      { 0.874662, 0.795188, 0.078150, 0.647681, -0.910413, 0.607686, 0.392656, 0.318523, 0.175609, -0.190211, -0.174326, -0.159084, -0.125848},
      { 0.684847, 0.781381, 0.173504, 0.743819, -0.308093, -0.019020, 0.201468, -0.032197, 0.421767, -0.184326, 0.085897, 0.056416, 0.027466},
      { -0.084307, 7.170500, -3.661860, -14.563892, 10.893159, -0.186497, -2.318544, -0.902224, 1.504792, 0.147942, -0.734568, 0.585001, -0.868378},
      { -4.121224, 2.466947, 2.325009, -11.400883, -10.773119, -0.627572, 0.455827, 1.057112, 1.796294, 0.987904, 0.212603, 0.359253, -0.105532},
      { -7.992438, -7.655343, -4.869693, -6.722327, 9.544512, -5.250600, -4.050820, -5.120951, -0.758755, 1.829706, 1.384338, 2.390245, 1.296190},
      { -10.218486, -0.629071, -8.504401, -8.641282, -2.207658, 1.818525, -1.423120, -0.898007, -1.263703, 2.076739, -2.244396, -0.292382, -0.353299},
      { 10.248974, -27.376506, 6.124142, 61.384787, -41.764771, 6.326810, 11.721932, 5.141960, -9.065378, 2.119422, 4.865250, -5.308071, 5.989284},
      { 32.326696, -13.531891, -19.457694, 44.769873, 42.491105, 6.557419, -3.201100, -8.098944, -12.725586, -5.451011, -1.217291, -5.300221, -0.130005},
      { 21.014798, 17.510967, 15.041109, 26.899854, -18.934167, 14.036661, 8.954179, 15.325031, 1.048105, -4.266107, -2.382286, -6.221658, -3.671553},
      { 27.778641, -9.561991, 31.240633, 11.989257, 20.109609, -3.582962, 5.319328, 6.014944, -0.156488, -6.828748, 5.770803, -0.508356, 0.585577},
      { -31.540378, 39.380786, -4.559957, -86.294496, 40.488945, -18.769312, -18.318774, -11.135692, 16.764698, -7.576188, -9.399571, 12.263502, -13.097443},
      { -82.664532, 39.293663, 40.663177, -52.462998, -52.547568, -21.780121, 8.544329, 14.784174, 28.032260, 8.528502, 0.715122, 14.568276, -0.115154},
      { -21.239112, -8.226969, -15.442636, -26.612158, 6.677141, -9.530594, -5.713634, -12.083189, -0.744603, 3.161936, 1.107489, 4.274360, 2.906592},
      { -23.982064, 18.410091, -24.831194, 1.868333, -23.761154, 1.421808, -4.591061, -6.513712, 1.590576, 6.079756, -3.733402, 1.369797, 0.036043},
      { 22.165010, -18.074133, 7.855068, 42.246652, -3.603180, 12.789308, 8.578457, 7.461044, -8.994677, 5.561167, 5.618378, -8.122258, 8.741300},
      { 64.192477, -38.786800, -22.642449, 15.698649, 22.567014, 19.725521, -6.242227, -7.074890, -18.533355, -4.179728, 0.706036, -11.385839, 0.759281},
      { 0.019447, -0.321519, 0.119784, -0.074797, 0.077260, -0.769781, -1.337296, 0.040636, -0.068956, -0.068883, 0.026387, 0.014205, -0.001321},
      { 0.068272, -0.184453, -0.026805, 0.001980, 0.023125, 1.282862, -0.762171, 0.045567, 0.054089, -0.013119, -0.065518, -0.050090, -0.033346},
      { 0.591252, 0.495495, -0.078775, 0.653448, -0.190878, 0.286104, 0.011563, 0.041493, 0.292102, 0.071813, -0.022038, 0.077655, 0.134747},
      { 0.642081, 0.298203, 0.174374, 0.035621, -0.027082, -0.021615, 0.253489, -0.125320, 0.176998, 0.024160, -0.156271, -0.106737, 0.078026},
      { 1.765835, 1.372020, 0.062259, 1.437948, -0.768550, 4.026957, 2.516653, -1.195206, 0.194463, 0.609759, 0.477914, 0.039275, 0.085043},
      { -2.207613, 2.529836, -0.687636, 0.351815, 0.384883, -2.611568, 3.505054, 0.163649, -1.327412, 0.090694, 0.614554, 0.601481, 0.095022},
      { -0.871535, 0.119544, -0.108937, -0.551275, 0.973566, 0.737487, 0.344844, -0.004828, -0.838485, 0.007011, 0.094322, -0.054031, -0.157722},
      { -1.761764, -0.965691, -1.114498, -0.734733, 0.424344, -0.315157, 0.747600, 0.077065, -0.250993, 0.150990, 0.335479, 0.320943, 0.003406},
      { -3.168587, -1.260433, -2.190761, -3.170990, 1.870636, -8.245646, -2.993538, 1.889443, 0.106361, -1.104209, -0.858865, -0.141571, -0.317136},
      { 4.019955, -2.289908, 1.225496, -0.670238, -1.628387, 3.373839, -7.201759, -0.466417, 2.089699, -0.028354, -0.812401, -0.886807, -0.142708},
      { -0.074023, -0.018372, 0.055160, -0.069238, 0.089849, -0.079363, -0.043138, 0.354535, -0.361675, -0.004009, 0.025287, 0.004072, -0.006672},
      { -0.046330, 0.185377, -0.085176, 0.030711, -0.012072, 0.032997, -0.046105, 0.335955, 0.334184, -0.039451, 0.005832, -0.008278, 0.018905},
      { -0.312693, 0.127976, -0.177347, 0.114024, 0.297272, -0.158177, 0.309353, -0.078180, 0.461209, -0.027827, -0.082803, 0.015608, -0.014203},
      { 0.086680, -0.094092, 0.193506, 0.010364, 0.030688, -0.248585, -0.152054, -0.442186, -0.069645, 0.003033, -0.084052, -0.048800, -0.021403},
      { 0.072451, 0.051953, -0.026251, 0.059080, 0.053744, 0.047860, -0.017435, 0.037730, -0.041852, 0.317511, 0.230828, -0.003490, -0.020955},
      { 0.030571, 0.126116, -0.099623, -0.062962, -0.006488, 0.025849, 0.113608, -0.020264, 0.059315, -0.187916, 0.244214, -0.020019, -0.024625},
      { 0.054342, -0.018074, 0.105125, 0.075732, 0.015922, 0.064729, -0.019431, -0.045231, -0.031091, 0.002592, 0.026699, -0.124771, 0.150817},
      { 0.097936, 0.088664, -0.017489, -0.049393, -0.016298, 0.065342, 0.020930, -0.004494, -0.012553, 0.019495, -0.021316, -0.210982, -0.127474},
      { -0.129283, -0.063425, -0.054209, 0.051204, -0.006596, 0.044107, -0.077901, -0.030469, 0.025592, 0.018597, -0.000246, -0.036534, -0.019517},
      { 0.028308, 0.103966, 0.083250, -0.221282, -0.103371, 0.011245, -0.082415, -0.028077, -0.003575, -0.036848, 0.036348, 0.010804, 0.030885},
      { -0.041841, 0.104022, 0.105598, -0.042406, -0.031386, -0.123089, -0.061930, 0.081084, 0.061734, 0.049359, -0.037290, 0.006585, -0.017415},
      { -0.058447, 0.082053, -0.067225, -0.093859, 0.062868, 0.029391, -0.013171, 0.054954, -0.044198, 0.012521, -0.027162, -0.027362, -0.024754}
    },
    {
      { 2.348018, -0.001699, 0.007715, 0.008054, 0.005956, 0.004442, 0.000192, -0.001132, 0.000421},
      { -0.000732, 0.104042, 0.073937, 0.006236, 0.017641, 0.002967, -0.017425, -0.001263, -0.011133},
      { 1.628826, 0.000839, -0.043487, 0.008183, -0.027630, -0.075356, -0.029852, 0.048012, -0.008284},
      { -0.363106, -0.590419, -0.212766, -0.041687, -0.092664, -0.047221, 0.033426, 0.003241, 0.026134},
      { -2.710255, 0.125712, 0.002491, -0.040918, 0.027339, 0.226196, 0.099359, -0.126889, -0.011937},
      { 0.351285, 0.449536, 0.186016, 0.047315, 0.073236, 0.046255, -0.022189, 0.005093, -0.015973},
      { 1.193932, -0.066091, 0.073483, 0.020656, -0.007722, -0.160789, -0.065105, 0.084813, 0.028908},
      { 0.022510, -0.475089, -0.204080, 0.000794, -0.006344, -0.006337, 0.005135, -0.000647, 0.005509},
      { -0.013830, 0.224803, -0.464797, -0.022138, 0.001138, 0.010352, -0.015882, -0.002307, 0.004197},
      { 0.047607, 0.234728, -0.007625, -0.033090, 0.018158, -0.018386, 0.012600, -0.007045, 0.033261},
      { 0.020139, 0.078512, 0.219390, 0.039412, -0.013902, 0.022683, -0.021540, -0.000198, 0.024949},
      { -0.487616, 1.574244, 0.432463, 0.218492, 0.075169, 0.075773, 0.018466, 0.029614, -0.096548},
      { -0.455430, -0.415314, 1.733171, 0.184123, -0.018706, -0.107045, 0.120685, -0.073586, -0.057949},
      { -0.293838, -1.884681, -0.076974, 0.175803, -0.233234, 0.085856, -0.433716, 0.049404, -0.332902},
      { 0.475885, 0.209720, -1.490486, -0.201115, 0.106034, -0.125392, 0.215934, 0.128539, -0.088779},
      { 1.327118, -1.829720, -0.109311, -0.762860, -0.061753, -0.152755, -0.023608, 0.121147, 0.345016},
      { 1.979343, -0.843470, -2.838183, -0.447563, 0.072140, 0.152487, -0.191963, 0.148621, 0.120824},
      { 0.644868, 3.963176, 1.131517, -0.365259, 0.210715, -0.198506, 1.393376, -0.096845, 0.899000},
      { -2.166833, -1.190547, 3.161021, 0.464725, -0.110211, 0.305447, -0.550121, -0.373833, 0.128468},
      { -0.934276, 1.199678, 0.501081, 0.580176, -0.081116, 0.068154, 0.046532, -0.205327, -0.287616},
      { -1.773041, 0.818055, 1.956153, 0.357749, -0.113235, -0.074916, 0.045023, -0.056935, -0.102033},
      { -0.463514, -2.643355, -1.685999, 0.371182, 0.034485, 0.115887, -1.149404, 0.027926, -0.657321},
      { 2.075349, 1.568890, -2.196361, -0.307358, 0.094175, -0.181613, 0.385548, 0.250442, -0.044992},
      { -0.015733, -0.014472, 0.018836, 0.095844, -0.082169, -0.004467, 0.002126, -0.001382, -0.004955},
      { -0.021777, -0.023440, 0.006277, 0.051548, 0.103019, 0.005133, -0.004611, -0.004134, -0.001079},
      { 0.106588, 0.069897, 0.019879, -0.035639, 0.147082, 0.023604, 0.005004, -0.023715, 0.000441},
      { 0.088662, 0.075925, -0.061172, -0.133245, 0.008753, 0.004629, 0.022664, -0.013934, 0.050403},
      { -0.005578, 0.124173, -0.136488, -0.637906, 0.563826, -0.011007, -0.010520, -0.084654, 0.087195},
      { -0.087516, 0.209648, 0.000548, -0.509944, -0.689913, -0.089406, 0.037529, -0.031821, 0.015336},
      { -0.296560, -0.399040, 0.135352, 0.293610, -0.554710, 0.004851, -0.013995, 0.163193, 0.017272},
      { -0.234823, -0.406313, 0.163453, 0.532010, 0.280655, 0.081339, -0.009131, 0.152424, -0.334780},
      { -0.020425, -0.377480, 0.190504, 0.300364, -1.050087, 0.020014, 0.056851, 0.159984, -0.122557},
      { 0.179915, -0.608465, -0.072259, 1.053742, 0.318071, 0.063549, -0.037475, 0.069399, -0.030931},
      { 0.060784, 0.654025, -0.227380, -0.210893, 0.326508, 0.031074, -0.028936, -0.226340, 0.010711},
      { 0.232391, 0.833718, -0.064806, -0.337450, -0.279959, -0.066289, 0.009593, -0.184023, 0.401939},
      { -0.026376, -0.001023, 0.020319, -0.003194, -0.016130, 0.032305, -0.067228, -0.008159, -0.008117},
      { -0.006698, -0.014285, -0.001860, 0.013103, -0.008092, 0.059979, 0.038945, 0.000080, -0.012524},
      { 0.014632, 0.065843, 0.002780, 0.005617, 0.015095, 0.052315, -0.009234, -0.008999, 0.003270},
      { 0.029486, 0.022521, 0.018000, -0.012775, -0.005724, 0.005642, 0.031011, 0.000398, 0.014850},
      { 0.084642, -0.069241, -0.042539, -0.019294, 0.011683, -0.139180, -0.059414, 0.010924, 0.041329},
      { 0.000852, 0.014060, -0.037598, 0.016026, 0.010226, 0.064456, -0.136401, -0.013533, 0.012086},
      { -0.017698, -0.019324, 0.009433, -0.008142, 0.001055, 0.000024, -0.000156, 0.023261, 0.014897},
      { -0.002064, 0.003572, -0.010381, 0.010619, 0.002215, -0.000543, 0.003015, -0.013645, 0.021360},
      { 0.017216, 0.015516, -0.013986, -0.015010, -0.005596, -0.009339, 0.018945, 0.035999, 0.065646},
      { 0.042485, -0.056088, 0.031171, -0.008497, -0.014708, -0.011700, -0.020747, -0.054002, 0.021964},
      { 0.002399, -0.005294, 0.009420, 0.006315, -0.005378, 0.004159, -0.001329, 0.001494, -0.003252},
      { 0.006756, 0.008762, -0.013714, 0.003111, 0.002457, 0.002003, 0.008298, 0.005674, 0.015190},
      { 0.009952, 0.005180, 0.003884, 0.007024, -0.008818, 0.001617, 0.002230, 0.006900, 0.008047},
      { -0.001120, 0.000586, -0.005133, 0.002056, 0.005241, 0.000906, -0.000496, -0.001411, 0.007141},
    }
  },
};

static const ITU_F2_layer_coefficients_fourier_test_t
  ITU_F2_layer_coefficients_fourier_test_vector[ITU_F2_LAYER_INTERPOLATED_COEFF_FOURIER_TEST_VECTORS_COUNT] =
{
  {
    186.328060,
    {0.0, 4},
    {
      12.449863, 2.409311, 54.089065, -57.298571, -261.913700, 312.086626, 479.737242, -642.019944, -427.937731, 569.899389, 150.245248, -185.248045, -1.514883, -0.798225, 2.088210, -0.430627,
      25.863680, -32.674075, -18.663771, 15.699147, -117.883190, 68.711532, 74.921172, -32.489332, 79.809261, 129.447469, -121.492935, -76.854825, 79.432526, -412.701131, 123.003527, 259.805297,
      -69.760607, 254.046342, -59.562025, -173.002943, 0.814153, -0.605680, -0.282469, 0.365118, 18.160237, -16.197021, 9.625684, -1.871229, -57.303215, 85.838296, -22.155819, 5.986576,
      44.894123, -157.217622, 7.648798, -12.960942, -3.743449, 97.164057, 1.217554, 0.966633, 0.916473, 0.598431, -1.780034, -6.487091, -1.224404, -1.553588, 4.534438, 11.127223,
      -0.389831, 0.420136, 0.382382, 0.068920, -0.123753, -0.199426, 0.077597, -0.040515, 0.002551, -0.084937, -0.035286, 0.043026
    },
    {2.346487, -0.050736, 1.666251, -0.250297, -2.796702, 0.244721, 1.206740, 0.220620, 0.472184, 0.094050, -0.166664, -0.959924, -2.385940, -0.349284, 1.767692, 1.743299,
     5.202454, -0.770309, -4.759476, -1.850621, -3.989485, 1.749054, 3.935344, -0.123819, 0.078496, 0.229228, 0.186326, 0.792451, -0.800171, -0.955354, -0.443270, -1.440425,
     0.576789,  0.654319, 0.409584, -0.003714, -0.064398, 0.039451, -0.010398, 0.239607, 0.197164, -0.011023, 0.028878, 0.072306, 0.039317, -0.014321, 0.029819, 0.003067,
     0.016890
    }
  },
  {
    186.738676,
    {10.333333, 4},
    {12.605497682,
     0.551506466, 47.022139774, -21.448664030, -232.123104595, 200.764231706, 409.461413670, -572.203653389, -336.313529859, 644.782789433, 105.878095472, -252.733077045, 0.509462478, 2.344543172, 2.421526994, 2.506997850, -27.765498792,
     -9.170952722, -30.901991624, -71.508784378, 120.170741210, 233.831852730, 92.168812032, 437.185394230, -111.905811833, -940.197494839, -51.277376062, -1076.988061263, -16.697104428, 1403.390170514, -119.352261133, 1125.602157218, 37.957453385,
     -698.152854904, 114.285189037, -416.092485976, 1.870004682, 0.601398662, -0.857661447, -0.182721559, 12.722215301, -2.923758620, 6.737100302, -13.432192542, -60.190568618, 24.488754670, -21.005718949, 64.898529127, 78.017091881,
     -66.946598203, 4.858141122, -71.035296381, -24.847382525, 60.239086024, 0.777315260, -1.254128297, -0.880149499, 0.866973682, -3.326071610, -1.238013274, -0.343299653, -1.549424458, 6.079231148, 1.600065539, -0.133203709,
     -0.650440630, -0.118109335, 1.051542442, -0.437364250, -0.009617647, 0.006056462, 0.233159653, -0.212611477, 0.136207528, 0.027188046, -0.079557094
      },
    {2.348583830, 0.024899411, 1.586116060, -0.288055376, -2.782106795, 0.288450491, 1.320131302, 0.041358209, -0.525517556, 0.000734276, 0.115188751, -0.964525882, 1.356872410, -0.037834939, -0.677714455, 2.512392935,
    -0.215168429, 0.903984898, 0.686483448, -1.286067035, -0.531922098, -1.461489809, -0.248006049, -0.111618606, 0.018803376, 0.219727474, 0.115543926, 0.747203747, -0.102341198, -0.762687644, -0.316864698, -0.735845853,
    -0.367649659, 0.136061313, 0.079677826, -0.054628257, -0.063472605, -0.050276834, 0.042014050, 0.199003049, -0.130970256, -0.019506269, -0.008617415, -0.027111264, 0.146933574, -0.000391404, -0.018168929, -0.008920463,
    -0.005064096,
    }
  }
};

static bool ITU_F2_layer_coefficients_test_new_AZr(
  F2_layer_fourier_coeff_context_t* const pContext,
  const NeQuickG_time_t * const pTime,
  double_t Azr,
  size_t solar_activity_index) {;
  if (F2_layer_fourier_coefficients_interpolate(
    pContext, pTime, Azr) != NEQUICK_OK) {
    return false;
  }

#ifdef FTR_MODIP_CCIR_AS_CONSTANTS
  F2_coefficient_array_t* pF2;
  if (CCIR_get_ionosonde_F2(pContext->time.month, &pF2) != NEQUICK_OK) {
    return false;
  }

  Fm3_coefficient_array_t* pFm3;
  if (CCIR_get_ionosonde_Fm3(pContext->time.month, &pFm3) != NEQUICK_OK) {
      return false;
  }
#endif // FTR_MODIP_CCIR_AS_CONSTANTS

  size_t degree;
  size_t order;
  for (degree = 0; degree < ITU_F2_COEFF_MAX_DEGREE; degree++) {
    for (order = 0; order < ITU_F2_COEFF_MAX_ORDER; order++) {
      double_t coef = pContext->interpolated.F2[degree][order];
      double_t coef_expected =
#ifdef FTR_MODIP_CCIR_AS_CONSTANTS
        pF2
#else
        pContext->ionosonde.F2
#endif
        [solar_activity_index][degree][order];
      if (!THRESHOLD_COMPARE(
            coef,
            coef_expected,
            ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON)) {
        LOG_ERROR("AF2 is not the expected.");
        return false;
      }
    }
  }

  for (degree = 0; degree < ITU_FM3_COEFF_MAX_DEGREE; degree++) {
    for (order = 0; order < ITU_FM3_COEFF_MAX_ORDER; order++) {
      double_t coef = pContext->interpolated.Fm3[degree][order];
      double_t coef_expected =
#ifdef FTR_MODIP_CCIR_AS_CONSTANTS
        pFm3
#else
        pContext->ionosonde.Fm3
#endif
        [solar_activity_index][degree][order];
      if (!THRESHOLD_COMPARE(
            coef,
            coef_expected,
            ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON)) {
        LOG_ERROR("AM3 is not the expected.");
        return false;
      }
    }
  }
  return true;
}

static bool ITU_F2_layer_coefficients_test_new_AZr_0(
  F2_layer_fourier_coeff_context_t* const pContext,
  const NeQuickG_time_t * const pTime) {
  // to satisfy lint
  size_t index = ITU_F2_LAYER_COEFF_LOW_SOLAR_ACTIVITY_IDX;
  return ITU_F2_layer_coefficients_test_new_AZr(
    pContext,
    pTime,
    NEQUICK_G_AZ_MIN_VALUE_SFU,
    index);
}

static bool ITU_F2_layer_coefficients_test_new_AZr_100(
  F2_layer_fourier_coeff_context_t* const pContext,
  const NeQuickG_time_t * const pTime) {
  // to satisfy lint
  size_t index = ITU_F2_LAYER_COEFF_HIGH_SOLAR_ACTIVITY_IDX;
  return ITU_F2_layer_coefficients_test_new_AZr(
    pContext,
    pTime,
    100.0,
    index);
}

static bool ITU_F2_layer_coefficients_test_new_AZr_interpolated(
  F2_layer_fourier_coeff_context_t* const pContext,
  const NeQuickG_time_t * const pTime) {
  for (size_t test_case_index = 0;
      test_case_index < ITU_F2_LAYER_INTERPOLATED_COEFF_TEST_VECTORS_COUNT;
    test_case_index++) {
    if (F2_layer_fourier_coefficients_interpolate(
      pContext,
      pTime,
      ITU_F2_layer_coefficients_test_vector[test_case_index].Azr) != NEQUICK_OK) {
      return false;
    }
    size_t degree;
    size_t order;
    for (degree = 0; degree < ITU_F2_COEFF_MAX_DEGREE; degree++) {
      for (order = 0; order < ITU_F2_COEFF_MAX_ORDER; order++) {
        double_t coef = pContext->interpolated.F2[degree][order];
        double_t coef_expected =
          ITU_F2_layer_coefficients_test_vector[test_case_index].AF2[degree][order];
        if (!THRESHOLD_COMPARE(
              coef,
              coef_expected,
              ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON)) {
          LOG_ERROR("AF2 is not the expected.");
          return false;
        }
      }
    }

    for (degree = 0; degree < ITU_FM3_COEFF_MAX_DEGREE; degree++) {
      for (order = 0; order < ITU_FM3_COEFF_MAX_ORDER; order++) {
        double_t coef = pContext->interpolated.Fm3[degree][order];
        double_t coef_expected =
          ITU_F2_layer_coefficients_test_vector[test_case_index].Am3[degree][order];
        if (!THRESHOLD_COMPARE(
              coef,
              coef_expected,
              ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON)) {
          LOG_ERROR("AM3 is not the expected.");
          return false;
        }
      }
    }
  }
  return true;
}

#ifdef FTR_MODIP_CCIR_AS_CONSTANTS
#define ITU_F2_layer_coefficients_interpolated_test(pCCIR_folder) \
  ITU_F2_layer_coefficients_interpolated_test()
#endif

static bool ITU_F2_layer_coefficients_interpolated_test(
  const char* const pCCIR_folder) {
  bool ret = true;
  F2_layer_fourier_coeff_context_t context;

  if (F2_layer_fourier_coefficients_init(
      &context,
      pCCIR_folder) != NEQUICK_OK) {
    return false;
  }

  NeQuickG_time_t time;
  time.month = 4;
  time.utc = 0;

  if (!ITU_F2_layer_coefficients_test_new_AZr_0(&context, &time)) {
    ret = false;
  }
  if (!ITU_F2_layer_coefficients_test_new_AZr_100(&context, &time)) {
    ret = false;
  }
  if (!ITU_F2_layer_coefficients_test_new_AZr_interpolated(&context, &time)) {
    ret = false;
  }

  F2_layer_fourier_coefficients_close(&context);
  return ret;
}

#ifdef FTR_MODIP_CCIR_AS_CONSTANTS
#define ITU_F2_layer_coefficients_fourier_test(pCCIR_folder) \
  ITU_F2_layer_coefficients_fourier_test()
#endif

static bool ITU_F2_layer_coefficients_fourier_test(
  const char* const pCCIR_folder) {
  bool ret = true;
  F2_layer_fourier_coeff_context_t context;

  if (F2_layer_fourier_coefficients_init(
      &context,
      pCCIR_folder) != NEQUICK_OK) {
    return false;
  }

  for (size_t i = 0; i < ITU_F2_LAYER_INTERPOLATED_COEFF_FOURIER_TEST_VECTORS_COUNT; i++) {
    solar_activity_t solar_activity;
    solar_activity.effective_sun_spot_count =
      ITU_F2_layer_coefficients_fourier_test_vector[i].Azr;

    if (F2_layer_fourier_coefficients_get(
      &context,
      &ITU_F2_layer_coefficients_fourier_test_vector[i].time,
      &solar_activity) != NEQUICK_OK) {
      return false;
    }

    for (size_t j = 0x00; j < ITU_F2_COEFF_MAX_DEGREE; j++) {
      if (!THRESHOLD_COMPARE(
            context.fourier.CF2[j],
            ITU_F2_layer_coefficients_fourier_test_vector[i].CF2[j],
            ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON)) {
        LOG_ERROR("CF2 is not the expected.");
        F2_layer_fourier_coefficients_close(&context);
        return false;
      }
    }

    for (size_t j = 0x00; j < ITU_FM3_COEFF_MAX_DEGREE; j++) {
      if (!THRESHOLD_COMPARE(
            context.fourier.Cm3[j],
            ITU_F2_layer_coefficients_fourier_test_vector[i].Cm3[j],
            ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON)) {
        LOG_ERROR("Cm3 is not the expected.");
        F2_layer_fourier_coefficients_close(&context);
        return false;
      }
    }
  }
  F2_layer_fourier_coefficients_close(&context);
  return ret;
}

bool ITU_F2_layer_coefficients_test(void) {
  bool ret = true;

#ifndef FTR_MODIP_CCIR_AS_CONSTANTS
  const char CCIR_folder[] = {"./../../ccir/"};
#endif

  if (!ITU_F2_layer_coefficients_interpolated_test(CCIR_folder)) {
    ret = false;
  }
  if (!ITU_F2_layer_coefficients_fourier_test(CCIR_folder)) {
    ret = false;
  }

  return ret;
}

#undef ITU_F2_LAYER_INTERPOLATED_COEFF_TEST_VECTORS_COUNT
#undef ITU_F2_LAYER_INTERPOLATED_COEFF_EPSILON
