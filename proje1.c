/*
                 Multi-threaded Matrix Multiplication
*/

#include <stdio.h> // giris cikis islemlerinin yapilmasini saglar  
#include <stdlib.h> //dinamik bellek yönetimi,rastgele sayi üretimi vb fonksiyonlarin yapilmasini saglar  
#include <time.h> // zaman bilgilerini depolamak icin kullanilir 
#include <sys/types.h> //typedef ile kendi türlerimizi tanimlamamizi saglar
#include <unistd.h> //POSIX standartıyla uyumluluk icin gerekli tanimlari icerir
#include <sys/mman.h> 
#include <string.h> //karakter dizileriyle alakali fonksiyonlari icerir
#include <pthread.h> //thread kavraminin c ile kodlanabilmesini saglar

// matris boyutlarini kontrol eder 
#define MATRIX_SIZE 5
 // normal bir diziyi iki boyutluymus gibi ele alir
#define array(arr, i, j) arr[(int) MATRIX_SIZE * (int) i + (int) j]

void fill_matrix(int *matrix);
void print_matrix(int *matrix, int print_width);
int *matrix_page(int *matrix, unsigned long m_size);
void matrix_unmap(int *matrix, unsigned long m_size);
__attribute__ ((noreturn)) void row_multiply(void *row_args);

// tüm matrisler icin integer isaretcileri tanimlanir
static int *matrix_a, *matrix_b, *matrix_c;

//her thread icin bagimsiz degisken yapisi
typedef struct arg_struct
{
  int *a;
  int *b;
  int *c;
  int row;
} thr_args;

//verilen matri 1'den 10'a kadar rastgele integer sayilar ile doldurur
void fill_matrix(int *matrix)
{
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      array(matrix, i, j) = rand() % 10 + 1;
    }
  }
  return;
}

//verilen matrisi yazdirir
void print_matrix(int *matrix, int print_width)
{
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j <  MATRIX_SIZE; j++)
    {
      printf("[%*d]", print_width, array(matrix, i, j));
    }
    printf("\n");
  }
  printf("\n");
  return;
}

//verilen matrisi mmap() kullanarak bir bellek sayfasina esler
int *matrix_page(int *matrix, unsigned long m_size)
{
  matrix = mmap(0, m_size, PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  // eger mmap() basarisizca cikar
  if (matrix == (void *) -1)
  {
    exit(EXIT_FAILURE);
  }
  memset((void *) matrix, 0, m_size);
  return matrix;
}

//verilen matrisin bellek sayfasindan eslemesini kaldirir
void matrix_unmap(int *matrix, unsigned long m_size)
{
  /// eger munmap() basarisizca cikar
  if (munmap(matrix, m_size) == -1)
  {
    exit(EXIT_FAILURE);
  }
}

//verilen satir icin tüm indeskleri hesaplamaya yarar
//noreturn ise derleyiciye bu fonksiyonun herhangi bir donusu olmayacagini bildirmek icin eklenir
__attribute__ ((noreturn)) void row_multiply(void *row_args)
{
  thr_args *args = (thr_args *) row_args;
  for(int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      int add = array(args->a, args->row, j) * array(args->b, j, i);
      array(args->c, args->row, i) += add;
    }
  }
  pthread_exit(0); //islevi cagiran diziyi sonlandirir
}

int main(void)
{
  //matrislerin bellek boyutu hesaplanir
  unsigned long m_size = sizeof(int) * (unsigned long) (MATRIX_SIZE * MATRIX_SIZE);

  // matrix_a, matrix_b ve matrix_c bir bellek sayfasina eslenir
  matrix_a = matrix_page(matrix_a, m_size);
  matrix_b = matrix_page(matrix_b, m_size);
  matrix_c = matrix_page(matrix_c, m_size);

  //her iki matris de 1-10 arası rastgele sayilarla doldurulur
  fill_matrix(matrix_a);
  fill_matrix(matrix_b);

  //her iki matris de yazdirilir
  printf("A Matrisi:\n---------\n");
  print_matrix(matrix_a, 2);
  printf("B Matrisi:\n---------\n");
  print_matrix(matrix_b, 2);

  //threadler icin malloc() fonksiyonu ile diziler tahsis edilir
  pthread_t *thrs;
  thr_args *args;
  if ((thrs = malloc(sizeof(pthread_t) * (unsigned long) MATRIX_SIZE)) == NULL ||
    (args = malloc(sizeof(thr_args) * (unsigned long) MATRIX_SIZE)) == NULL)
  {
    exit(EXIT_FAILURE);
  }

  //threadler olusturulur
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    args[i] = (thr_args) {
      .a = matrix_a,
      .b = matrix_b,
      .c = matrix_c,
      .row = i
    };
    pthread_create(&thrs[i], NULL, (void *) &row_multiply, (void *) &args[i]);
  }

  //her threadin yaptigi is toplanir
  for (int j = 0; j < MATRIX_SIZE; j++)
    pthread_join(thrs[j], NULL);

  //her thread icin ayrilan alanlar bosaltilir
  if (thrs != NULL)
  {
    free(thrs);
    thrs = NULL;
  }
  if (args != NULL)
  {
    free(args);
    args = NULL;
  }

  //carpma istemini sonucu olarak C matrisi yazdirilir
  printf("Matris Carpiminin sonucu C Matrisi:\n--------------\n");
  print_matrix(matrix_c, 4);

  //bellek sayfalarindan ayrilan alanlar geri verilir
  matrix_unmap(matrix_a, m_size);
  matrix_unmap(matrix_b, m_size);
  matrix_unmap(matrix_c, m_size);
}
