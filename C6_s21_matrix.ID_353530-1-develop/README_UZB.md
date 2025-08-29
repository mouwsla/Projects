# s21_matrix

matrix.h. kutubxonasini amalga oshirish 

💡 **Ushbu loyiha haqida biz bilan fikr-mulohazalaringizni baham ko’rish [uchun bu yerni bosing](https://new.oprosso.net/p/4cb31ec3f47a4596bc758ea1861fb624)**. Bu anonim bo’lib, jamoamizga ta’limni yaxshilashga yordam beradi. Loyihani tugatgandan so'ng darhol so'rovnomani to'ldirishni tavsiya qilamiz.

## Contents

1. [Chapter I](#chapter-i) \
    1.1. [Introduction](#introduction)
2. [Chapter II](#chapter-ii) \
    2.1. [Information](#information)
3. [Chapter III](#chapter-iii) \
    3.1. [Part 1](#part-1-реализация-функции-библиотеки-matrixh) 


# Chapter I

![matrix](misc/rus/images/matrixx.png)

Yer sayyorasi, 2000-yil 13-sentabr.

*- Biroq, bizning SEO ajoyib shahar tashqarisidagi uyga ega, g'oyalarni hayotga tatbiq etish uchun hamma narsa bor! Maysazordagi ulkan basseyn va unga qaragan ayvon ishtiyoqli va aqlli inson qiyofasini yanada to'ldiradi.*

*— Ha, fikringga qo’shilaman, bizni bu yerga taklif qilishganidan juda xursandman, ajoyib joy!*

*- Shubhasiz! Shunday qilib, bir necha kundan beri undagi id Software asosiy texnik komandasi biz ishlab chiqayotgan Doom 3 o'yinimizga kiritmoqchi bo'lgan yangi texnologiyani muhokama qilmoqda.*
*Fotosuratdagi reallik hissini ko’proq nima yaratadi? Tabiiyki, hozir hisoblash uchun juda ko'p vaqt talab qiladigan yorug'lik va soyaning o'ynashi markaziy protsessorni og'ir yuklaydi. Jon o'zining texnologik va algoritmik g'oyalari va nayranglari bilan mashhur bo'lib, unga kod ishlashi va optimallashtirishda aqldan ozgan yutuqlarga erishishga imkon berdi.*
*Men nima haqida gapiryapman... Bizning bosh texnik mutaxassisimiz va kompaniya asoschisi Jon Karmak chuqurlik buferi va trafaret buferi yordamida butun grafik quvur liniyasidan o‘tgandan so‘ng sahnada soyalarni qo‘llash imkonini beradigan nazariy ishlanmani taqdim etdi.*

*- O, zo’r-ku, etim jimirlab ketyapti, batafsilroq aytib bersangchi!*

*- Sizni ushbu ziyofatga tasodifan taklif qilganimiz yo'q, butun jamoa sahnada soyalarni yaratishning yangi usuli ustida ishlamoqda va ayniqsa sizning bo'lim Jon tomonidan barcha turdagi matritsalarning eng tez va optimallashtirilgan kutubxonasini joriy qilish uchun tayinlangan. Algoritmning butun matematik mantig'i quriladigan transformatsiyalar: vektorlar va matritsalar, transpozitsiya va SRT transformatsiyasi, shuningdek, kompyuter grafikasida qo'llaniladigan boshqa ko'plab matematik obyektlar va operatsiyalar. Yangi usulga to'g'ri va qasddan o'tish uchun ishlashda sezilarli va ta'sirchan o'zgarishlar talab qilinadi, buning uchun siz shaxsan javobgar bo'lasiz!*

*— Mening jamoam va men sizga yordam berishdan juda xursandmiz va ertagayoq boshlashga tayyormiz!*

*–  Ajoyib! Kim biladi, balki bir kun yorug'lik va soyalar uchun faqat nurlarni yuborish kifoya qiladi ... lekin hozircha biz o'z zamonamizning texnologiyalari bilan cheklanganmiz va shu bilan kifoyalanishimiz kerak. Shunday ekan, olg’a! Va ha, dedlaynlarni o’tkazib yubora ko’rmang, u buni yoqtirmaydi*

## Introduction

Ushbu loyihada siz C dasturlash tilida raqamli matritsalarni qayta ishlash uchun kutubxonangizni amalga oshirishingiz kerak. Matritsalar dasturlashdagi asosiy ma'lumotlar tuzilmalaridan biridir, masalan, ular jadval qiymatlarini ifodalash, hisoblash muammolari va neyron tarmoqlari uchun ishlatiladi. Ushbu loyiha doirasida siz matritsalar bilan tanishasiz va tizimli yondashuvni mustahkamlaysiz.


# Chapter II

## Information

## Tarixiy ma'lumot

Matritsalar haqida birinchi eslatmalar (yoki ular o'sha paytda "sehrli kvadratlar" deb nomlangan) qadimgi Xitoy hududida topilgan.

Ular XVIII asr o'rtalarida mashhur matematik Gabriel Kramerning "algebraik egri chiziqlarni tahlil qilishga kirish" asarini nashr etganligi tufayli mashhur bo'lishdi, unda chiziqli tenglamalar tizimini yechishning yangi algoritmi tasvirlangan. Ko'p o'tmay, Karl Fridrix Gaussning chiziqli tenglamalarni yechishning "klassik" usuli, Hamilton-keli teoremasi, Karl Veyerstrass, Georg Frobenius va boshqa taniqli olimlarning asarlari nashr etildi.

Va faqat 1850 yilda Jeyms Jozef Silvestr o'z ishida "matritsa" atamasini kiritdi.

## Matritsa

Matritsa – bu qatorlar va ustunlarning belgilangan sonida joylashgan raqamlar to'plami. 

Matritsa A – bu m qatorlarda va n ustunlarda joylashgan raqamlarning to'rtburchaklar jadvali

```
    1 2 3
A = 4 5 6
    7 8 9
```

```
     1  2  3  4
В =  5  6  7  8
     9 10 11 12
```

Kerakli elementni indekslar yordamida olinishi mumkin: A[1,1] = 1, bu yerda birinchi indeks qator raqami, ikkinchisi ustun raqami.

A matritsasi quyidagi indekslari mavjud elementlarga ega bo'ladi:

```
    (1,1) (1,2) (1,3)
A = (2,1) (2,2) (2,3)
    (3,1) (3,2) (3,3)
```

Matritsaning tartibi-bu uning qatorlari yoki ustunlari soni. \
Kvadrat matritsaning asosiy diagonali yuqori chapdan pastki o'ng burchakka cho'zilgan diagonaldir. \
To'rtburchak matritsa (B) – qatorlar soni ustunlar soniga teng bo'lmagan matritsa.\ 
Kvadrat matritsa (A) — qatorlar soni ustunlar soniga teng bo'lgan matritsa.

Ustun matritsasi – bu faqat bitta ustunga ega bo'lgan matritsa:

```
    (1,1)
A = (2,1)
    (n,1)
```

Qator matritsasi – bu faqat bitta qatorga ega matritsa:

```
A = (1,1) (1,2) (1,m)
```

Tip: ustun matritsasi va qator matritsasi ko'pincha vektorlar deb ham ataladi.

Diagonal matritsa – bu asosiy diagonaldan tashqaridagi barcha elementlar nolga teng bo'lgan kvadrat matritsa. \
Birlik matritsasi – diagonal matritsa bo'lib, unda barcha diagonal elementlar birga teng:

```
    1 0 0
A = 0 1 0
    0 0 1
```

Uchburchak matritsa – bu asosiy diagonalning bir tomonida joylashgan barcha elementlar nolga teng bo'lgan kvadrat matritsa

```
    1 2 3
A = 0 4 5
    0 0 6
```

### C tilidagi matritsaning tuzilishi:

```c
typedef struct matrix_struct {
    double** matrix;
    int rows;
    int columns;
} matrix_t;
```

## Matritsalar bo'yicha operatsiyalar

Barcha operatsiyalar (matritsalarni taqqoslashdan tashqari) olingan kodni qaytarishi kerak:
- 0 – ОК;
- 1 — Xato, noto’g’ri matritsa;
- 2 — Hisoblash xatosi (matritsalarning mos kelmaydigan o'lchamlari; hisoblash mumkin bo'lmagan matritsa va boshqalar).

### Matritsalarni yaratish (create_matrix)

```c
int s21_create_matrix(int rows, int columns, matrix_t *result);
```

### Matritsalarni tozalash (remove_matrix)

```c
void s21_remove_matrix(matrix_t *A);
```

### Matritsalarni taqqoslash (eq_matrix)

```c
#define SUCCESS 1
#define FAILURE 0

int s21_eq_matrix(matrix_t *A, matrix_t *B);
```

Ikkita A, B matritsalari agar ularning o'lchamlari mos kelsa va mos keladigan elementlar teng bo'lsa mos tushadi |A = B|, ya'ni barcha i, j A(i,j) = B(i,j).

Taqqoslash verguldan keyin yettinchi belgigacha, uni ham o’z ichiga olgan holda amalga oshirilishi kerak.

### Matritsalarni qo'shish (sum_matrix) va ayirish (sub_ matrix)

```c
int s21_sum_matrix(matrix_t *A, matrix_t *B, matrix_t *result);
int s21_sub_matrix(matrix_t *A, matrix_t *B, matrix_t *result);
```

Bir xil o'lchamdagi ikkita A = m x n va B = m x n matritsalarining yig'indisi bir xil o'lchamdagi C = m x n = A + B matritsa bo'lib, uning elementlari C(i,j) = A(i,j) + B(i,j ) tengliklari bilan aniqlanadi. 

Bir xil o'lchamdagi ikkita A = m x n B = m x n matritsalar ayirmasi bir xil o'lchamdagi C = m x n = A - B matritsasi bo'lib, uning elementlari C(i,j) = A(i, j) - B(i,j) tengliklari bilan aniqlanadi. 

```
            1 2 3   1 0 0   2 2 3
С = A + B = 0 4 5 + 2 0 0 = 2 4 5
            0 0 6   3 4 1   3 4 7
```

### Matritsani songa ko'paytirish (mult_number). Ikki matritsani ko'paytirish (mult_matrix)

```c
int s21_mult_number(matrix_t *A, double number, matrix_t *result);
int s21_mult_matrix(matrix_t *A, matrix_t *B, matrix_t *result);
```

A = m x n matritsaning λ soniga ko‘paytmasi B = m x n = λ x A matritsa bo‘lib, uning elementlari B = λ x A(i,j) tengliklari bilan aniqlanadi.

```
                1 2 3   2 4 6   
B = 2 × A = 2 × 0 4 2 = 0 8 4 
                2 3 4   4 6 8   
```

A = m x k matritsaning B = k x n matritsaga ko'paytmasi m x n o'lchamdagi C = m x n = A x B matritsa bo'lib, uning elementlari C(i,j) = A(i, 1) x B( 1,j) + A(i,2) x B(2,j) + ... + A(i,k) x B(k,j) tengligi bilan aniqlanadi. 

```
            1 4    1 -1  1    9 11 17   
C = A × B = 2 5  × 2  3  4 = 12 13 22 
            3 6              15 15 27
```

C matritsasining komponentlari quyidagicha hisoblanadi:

```
C(1,1) = A(1,1) × B(1,1) + A(1,2) × B(2,1) = 1 × 1 + 4 × 2 = 1 + 8 = 9
C(1,2) = A(1,1) × B(1,2) + A(1,2) × B(2,2) = 1 × (-1) + 4 × 3 = (-1) + 12 = 11
C(1,3) = A(1,1) × B(1,3) + A(1,2) × B(2,3) = 1 × 1 + 4 × 4 = 1 + 16 = 17
C(2,1) = A(2,1) × B(1,1) + A(2,2) × B(2,1) = 2 × 1 + 5 × 2 = 2 + 10 = 12
C(2,2) = A(2,1) × B(1,2) + A(2,2) × B(2,2) = 2 × (-1) + 5 × 3 = (-2) + 15 = 13
C(2,3) = A(2,1) × B(1,3) + A(2,2) × B(2,3) = 2 × 1 + 5 × 4 = 2 + 20 = 22
C(3,1) = A(3,1) × B(1,1) + A(3,2) × B(2,1) = 3 × 1 + 6 × 2 = 3 + 12 = 15
C(3,2) = A(3,1) × B(1,2) + A(3,2) × B(2,2) = 3 × (-1) + 6 × 3 = (-3) + 18 = 15
C(3,3) = A(3,1) × B(1,3) + A(3,2) × B(2,3) = 3 × 1 + 6 × 4 = 3 + 24 = 27			
```

### Matritsaning transpozitsiyasi (transpose)

```c
int s21_transpose(matrix_t *A, matrix_t *result);
```

A matritsasining transpozitsiyasi bu matritsa qatorlarini raqamlarini saqlab qolgan holda ustunlari bilan almashtirishdan iborat.

```
          1 4   1 2 3
A = A^T = 2 5 = 4 5 6
          3 6
```

### Matritsa minori va algebraik qo’shimchalar matritsasi (calc_complements)

```c
int s21_calc_complements(matrix_t *A, matrix_t *result);
```

А i qatoridan va j ustunidan ayrilishidan olingan (n-1) tartibining aniqlovchisi Minor M (i, j) deb ataladi.

Matritsa uchun:

```
    1 2 3
A = 0 4 2
    5 2 1
```

Birinchi satrdagi birinchi elementning minori: 

```
M(1,1) = 4 2
         2 1

|M| = 4 - 4 = 0
```

Minorlar matritsasi quidagi ko’rinishga ega:

```
     0 -10 -20
M = -4 -14  -8
    -8   2   4
```

Matritsa elementining algebraik to‘ldiruvchisi -1^(i+j) ga ko‘paytirilgan minor qiymatdir.

Algebraik qo'shimchalar matritsasi quidagi ko’rinishga ega:

```
      0  10 -20
M. =  4 -14   8
     -8  -2   4
```

Matritsa aniqlovchisi (determinant)

```c
int s21_determinant(matrix_t *A, double *result);
```

Aniqlovchi (determinant) – har bir kvadrat matritsaga tayinlangan va maxsus formulalar yordamida elementlardan hisoblangan raqam.

Tip: determinant faqat kvadrat matritsa uchun hisoblanishi mumkin.

Matritsaning determinanti qator (ustun) elementlarining tegishli algebraik to'ldiruvchilarning mahsuloti yig'indisiga teng.

Birinchi qator yordamida A matritsa uchun determinantni topish:

```
    1 2 3
A = 4 5 6
    7 8 9
	
|A| = 1 × 5 6 - 2 × 4 6 + 3 × 4 5 = 1 × (5 × 9 - 8 × 6) - 2 × (4 × 9 - 6 × 7) + 3 × (4 × 8 - 7 × 5)
          8 9       7 9       7 8
|A| = 1 × (45 - 48) - 2 × (36 - 42) + 3 × (32 - 35) = -3 + 12 + (-9) = 0
|A| = 0
```

### Teskari matritsa (inverse_matrix)

```c
int s21_inverse_matrix(matrix_t *A, matrix_t *result);
```

Agar bu matritsalarning ko'paytmasi bir xillik matritsasiga teng bo'lsa, -1 darajali A matritsa A kvadrat matritsaga teskari deyiladi.

Agar determinant 0 bo'lsa, teskari matritsa mavjud bo’lmaydi.

Teskari matritsa $`A^{-1}=\frac{1} {|A|} × A_*^T`$  formulasi boʻyicha topiladi.

Quidagi matritsa berilgan:

```
     2  5  7
A =  6  3  4
     5 -2 -3
```

Determinantni qidirish:

``` |A| = -1 ```

Determinant |A| != 0 -> teskari matritsa mavjud.

Minorlar matritsasi tuzilishi:

```
    -1 -38 -27
М = -1 -41 -29
    -1 -34 -24
```

Algebraik qo'shimchalar matritsasi teng bo'ladi:

```
     -1  38 -27
М. =  1 -41  29
     -1  34 -24
```

Algebraik qo'shimchalarning ko'chirilgan matritsasi teng bo'ladi:

```
        -1   1  -1
М^T. =  38 -41  34
       -27  29 -24
```

Teskari matritsa quyidagi ko'rinishga ega bo'ladi:

```
                           1  -1   1
A^(-1) =  1/|A| * M^T. = -38  41 -34
                          27 -29  24 
```

# Chapter III

## Part 1. matrix.h kutubxonasi funksiyasini amalga oshirish

Matritsalar bilan asosiy amallarni bajaring (qisman [yuqorida](#matritsalar-boyicha-operatsiyalar) tavsiflangan): create_matrix (yaratish), remove_matrix (tozalash va yo'q qilish), eq_matrix (taqqoslash), sum_matrix (qo'shish), sub_matrix (ayirish), mult_matrix (ko'paytirish), mult_number (raqamga ko'paytirish), transpose (ko'chirish), determinant (aniqlovchini hisoblash), calc_complements (algebraik qo'shimchalar matritsasini hisoblash), inverse_matrix (teskari matritsani qidirish).

- Kutubxona gcc kompilyatoridan foydalangan holda C11 standartidagi C tilida ishlab chiqilishi kerak
- Kutubxona kodi develop tarmog’odagi src papkasida bo'lishi kerak
- Eskirgan yoki iste’moldan chiqarilgan til konstruksiyalari va kutubxona funksiyalaridan foydalanmang. Til va kutubxonalar haqidagi rasmiy hujjatlardagi legacy va obsolete belgilarga e'tibor bering. POSIX.1-2017 standartiga e'tibor qarating.
- Kod yozishda siz Google Style ga amal qilishingiz kerak
- Yechimni s21_matrix.a nomli statik kutubxona sifatida loyihalashtiring (s21_matrix.h sarlavha fayli bilan).
- Kutubxona tuzilgan dasturlash tamoyillariga muvofiq ishlab chiqilishi kerak.
- Har bir funksiyadan oldin s21_ prefiksidan foydalaning.
- Check kutubxonasidan foydalanib, kutubxona funksiyalarining to'liq unit testlarini tayyorlang
- Unit testlari har bir funksiyaning kamida 80% ni qamrab olishi kerak.
- Kutubxona va testlarni yaratish uchun Makefile faylini nazarda tuting (all, clean, test, s21_matrix.a, gcov_report maqsadlari bilan)
-  gcov_report maqsadi html sahifasi shaklida gcov hisobotini yaratishi kerak. Buning uchun unit testlari gcov bayroqlari bilan bajarilishi kerassk
- Matritsa [yuqorida](#c-tilidagi-matritsaning-tuzilishi) tavsiflangan struktura sifatida amalga oshirilishi kerak.
- Kasr qismining tekshiriladigan aniqligi –  verguldan keyin ko’pi bilan 6 ta belgi
