int k;



int params_mix(float x0, int i1[], int i2, float x3[], float x4, int i5,
               float x6, float x7, float x8[], int i9[], int i10, int i11,
               float x12[], int i13[], int i14[], int i15, float x16[],
               float x17[], float x18, float x19, float x20, float x21[],
               int i22, float x23, float x24, float x25, int i26[], float x27[],
               int i28[], int i29[], float x30[], float x31, float x32,
               int i33[], int i34, float x35[], float x36[], float x37,
               float x38, int i39[], int i40[], int i41, int i42, float x43,
               float x44, int i45[], int i46, float x47[], int i48, int i49[],
               int i50[], float x51, float x52, float x53[], int i54, int i55[],
               float x56[], float x57, int i58, float x59, float x60[],
               float x61[], float x62, int i63) {
  float arr[10] = {x0 + x3[k] + x4 + x6,        x7 + x8[k] + x12[k] + x16[k],
                   x17[k] + x18 + x19 + x20,    x21[k] + x23 + x24 + x25,
                   x27[k] + x30[k] + x31 + x32, x35[k] + x36[k] + x37 + x38,
                   x43 + x44 + x47[k] + x51,    x52 + x53[k] + x56[k] + x57,
                   x59 + x60[k] + x61[k] + x62};

    putfarray(10, arr);
  putch(10);

  int arr2[10] = {
      i1[k] + i2 + i5,       i9[k] + i10 + i11,     i13[k] + i14[k] + i15,
      i22 + i26[k] + i28[k], i29[k] + i33[k] + i34, i39[k] + i40[k] + i41,
      i42 + i45[k] + i46,    i48 + i49[k] + i50[k], i54 + i55[k] + i58 + i63};
  putint(i54 + i55[k] + i58 + i63);
      
  if (i63 != 0) {
    int i = 0;
    putarray(10, arr2);
    while (i < 10) {
      arr2[i] = arr2[i] - arr[i];
      i = i + 1;
    }
    return arr2[k] * arr[8];
  } else {
    return params_mix(x0, arr2, i2, arr, x4, i5, x6, x7, x8, i9, i10, i11, x12,
                      i13, i14, i15, x16, x17, x18, x19, x20, x21, i22, x23,
                      x24, x25, i26, x27, i28, i29, x30, x31, x32, i33, i34,
                      x35, x36, x37, x38, i39, i40, i41, i42, x43, x44, i45,
                      i46, x47, i48, i49, i50, x51, x52, x53, i54, i55, x56,
                      x57, i58, x59, x60, x61, i63, x62);
  }
}

int main() {
  float arr[40][3];
  int arr2[24][3], i;

  k = getint();
  i = 0;
  while (i < 40) {
    getfarray(arr[i]);
    i = i + 1;
  }
  i = 0;
  while (i < 24) {
    getarray(arr2[i]);
    i = i + 1;
  }


  int ret3 = params_mix(
      arr[0][k], arr2[0], arr2[1][k], arr[1], arr[2][k], arr2[2][k], arr[3][k],
      arr[4][k], arr[5], arr2[3], arr2[4][k], arr2[5][k], arr[6], arr2[6],
      arr2[7], arr2[8][k], arr[7], arr[8], arr[9][k], arr[10][k], arr[11][k],
      arr[12], arr2[9][k], arr[13][k], arr[14][k], arr[15][k], arr2[10],
      arr[16], arr2[11], arr2[12], arr[17], arr[18][k], arr[19][k], arr2[13],
      arr2[14][k], arr[20], arr[21], arr[22][k], arr[23][k], arr2[15], arr2[16],
      arr2[17][k], arr2[18][k], arr[24][k], arr[25][k], arr2[19], arr2[20][k],
      arr[26], arr2[21][k], arr2[22], arr2[23], arr[27][k], arr[28][k], arr[29],
      arr2[0][k], arr2[1], arr[30], arr[31][k], arr2[2][k], arr[32][k], arr[33],
      arr[34], arr[35][k], arr2[3][k]);


  putch(10);
  putint(ret3);
  putch(10);
  return 0;
}
