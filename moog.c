#include <math.h>
#include <memory.h>
#include <stdio.h>

#define polyin float
#define polyout float

#define BUFSIZE 64

float delta_func[BUFSIZE];
float out_buffer[BUFSIZE];

void tick(float in, float cf, float reso, float *out) {

  // start of sm code

  // filter based on the text "Non linear digital implementation of the moog
  // ladder filter" by Antti Houvilainen adopted from Csound code at
  // http://www.kunstmusik.com/udo/cache/moogladder.udo
  polyin input;
  polyin cutoff;
  polyin resonance;

  polyout sigout;

  // remove this line in sm
  input = in;
  cutoff = cf;
  resonance = reso;

  // resonance [0..1]
  // cutoff from 0 (0Hz) to 1 (nyquist)

  float pi;
  pi = 3.1415926535;
  float v2;
  v2 = 40000; // twice the 'thermal voltage of a transistor'
  float sr;
  sr = 22100;

  float cutoff_hz;
  cutoff_hz = cutoff * sr;

  static float az1;
  static float az2;
  static float az3;
  static float az4;
  static float az5;
  static float ay1;
  static float ay2;
  static float ay3;
  static float ay4;
  static float amf;

  float x; // temp var: input for taylor approximations
  float xabs;
  float exp_out;
  float tanh1_out, tanh2_out;
  float kfc;
  float kf;
  float kfcr;
  float kacr;
  float k2vg;

  kfc = cutoff_hz / sr; // sr is half the actual filter sampling rate
  kf = cutoff_hz / (sr * 2);
  // frequency & amplitude correction
  kfcr =
      1.8730 * (kfc * kfc * kfc) + 0.4955 * (kfc * kfc) - 0.6490 * kfc + 0.9988;
  kacr = -3.9364 * (kfc * kfc) + 1.8409 * kfc + 0.9968;

  x = -2.0 * pi * kfcr * kf;
  exp_out = expf(x);

  k2vg = v2 * (1 - exp_out); // filter tuning

  // cascade of 4 1st order sections
  float x1 = (input - 4 * resonance * amf * kacr) / v2;
  float tanh1 = tanhf(x1);
  float x2 = az1 / v2;
  float tanh2 = tanhf(x2);
  ay1 = az1 + k2vg * (tanh1 - tanh2);

  // ay1  = az1 + k2vg * ( tanh( (input - 4*resonance*amf*kacr) / v2) -
  // tanh(az1/v2) );
  az1 = ay1;

  ay2 = az2 + k2vg * (tanh(ay1 / v2) - tanh(az2 / v2));
  az2 = ay2;

  ay3 = az3 + k2vg * (tanh(ay2 / v2) - tanh(az3 / v2));
  az3 = ay3;

  ay4 = az4 + k2vg * (tanh(ay3 / v2) - tanh(az4 / v2));
  az4 = ay4;

  // 1/2-sample delay for phase compensation
  amf = (ay4 + az5) * 0.5;
  az5 = ay4;

  // oversampling (repeat same block)
  ay1 = az1 + k2vg * (tanh((input - 4 * resonance * amf * kacr) / v2) -
                      tanh(az1 / v2));
  az1 = ay1;

  ay2 = az2 + k2vg * (tanh(ay1 / v2) - tanh(az2 / v2));
  az2 = ay2;

  ay3 = az3 + k2vg * (tanh(ay2 / v2) - tanh(az3 / v2));
  az3 = ay3;

  ay4 = az4 + k2vg * (tanh(ay3 / v2) - tanh(az4 / v2));
  az4 = ay4;

  // 1/2-sample delay for phase compensation
  amf = (ay4 + az5) * 0.5;
  az5 = ay4;

  sigout = amf;

  // end of sm code

  *out = sigout;

} // tick

int main(int argc, char *argv[]) {

  // set delta function
  memset(delta_func, 0, sizeof(delta_func));
  delta_func[0] = 1.0;

  int i = 0;
  for (i = 0; i < BUFSIZE; i++) {
    tick(delta_func[i], 0.6, 0.7, out_buffer + i);
  }
  for (i = 0; i < BUFSIZE; i++) {
    printf("%f;", out_buffer[i]);
  }
  printf("\n");

} // main
