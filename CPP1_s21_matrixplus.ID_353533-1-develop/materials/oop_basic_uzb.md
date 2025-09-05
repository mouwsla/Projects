# Obyektga yo’naltirilgan dasturlashning asosiy qoidalari 

Obyektga yo’naltirilgan dasturlash – dasturni ierarxik tuzilishda joylashtirilgan ma'lum sinflarga mansub o'zaro ta'sir qiluvchi axborot obyektlari to’plami sifatida ko'rsatishga asoslangan dasturlash metodologiyasi.

**Sinf** - ularda aniqlangan bir xil xossalar va usullar to'plamiga ega bo'lgan ba'zi obyektlar to’plami.

**Obyekt** – sinf aniq bir nusxasi 

Haqiqiy dunyoning deyarli har qanday qismi ushbu qismni dasturchi uchun zarur bo'lgan nuqtai nazardan tavsiflovchi ma'lum bir rasmiylashtirilgan obyektlar to'plami bilan ifodalanishi mumkin. Ilovalarni ishlab chiqishga bunday yondashuv obyektga yo’naltirilgan yondashuv deb ataladi. Siz hozir bo'lgan joy ham "joy" sinfining obyektidir. Bundan tashqari, agar bu joy turli shahar muassasalari to'g'risida ma'lumot beruvchi ariza doirasida ko'rib chiqilsa, ushbu obyekt bilan faqat manzilga va muayyan turdagi muassasaga, ko'rsatiladigan xizmatlar to'plamiga va hokazolarga mos keladigan xususiyatlarni bog'lash kifoya. Agar bu joy 2D xaritani planar ko'rsatish dasturi doirasida hisobga olinsa, sizga joylashuv va o'lchamlar kabi bir oz boshqacha xususiyatlar kerak bo'ladi. Agar siz ekranda ma'lum bir joyni uch o'lchamli shaklda tasavvur qilmoqchi bo'lsangiz, sizga ushbu joyda joylashgan obyektlarning vizual xususiyatlari, ularning joylashishi va bir-biriga nisbatan o'lchamlari, yorug'lik manbalari va boshqalarning aniq tavsiflari ro'yxati kerak bo'ladi. Shunday qilib, real yoki mavhum dunyoning bir xil obyekti faqat zarur bo'lgan xususiyatlarni saqlab qolgan holda, dasturiy ta'minot darajasida butunlay boshqacha tarzda ifodalanishi yoki simulyatsiya qilinishi mumkin. Aynan shu xususiyatlar to’plami asosan sinflarning o'zini belgilaydi.

Obyektga yo’naltirilgan dasturlashning asosiy tamoyillari:

- Abstraksiya – ishlab chiqilayotgan dastur nuqtai nazaridan obyektni yetarlicha to'liq aniqlaydigan obyekt xususiyatlarining minimal sonini ajratib ko'rsatish.

- Inkapsulyatsiya – sinf ichida amalga oshirishni yashirish va sinf bilan o’zaro aloqa qilish uchun tashqi interfeysni ta’minlash.

- Meros - mavjud sinflarga asoslangan yangi sinflarning ierarxik avlodi, ota-ona klassni amalga oshirishdan to'liq yoki qisman qayta foydalanish.

- Polimorfizm - turli xil ilovalarga ega bo'lgan bir xil interfeysni amalga oshiradigan turli sinflarning imkoniyati.


Abstraksiya tamoyilidan kelib chiqqan holda, obyektga yo'naltirilgan dasturlash ko'proq "tabiiy" parchalanish va abstraktsiyalarni ajratib olishga olib keladi. Masalan, matritsa. Strukturaviy yondashuvda strukturaviy matritsa strukturasini va ushbu tuzilma bilan ishlash uchun barcha kerakli funktsiyalarni aniqlash kerak. Shu bilan birga, Obyektga yo’naltirilgan dasturlash metodologiyasida bu muammo boshqacha hal qilinadi: obyektlarning yangi sinfi - matritsa yaratiladi, u bilan usullar deb ataladigan ba’zi maydonlar va funktsiyalar bog'lanadi. Ushbu yondashuv bilan matritsaning barcha xarakteristikalari ba'zi sinflarni amalga oshirishda qamrab olinadi va dasturda mavjud bo'lganda matritsa obyektining o'zi orqali ishlatilishi mumkin.


# Istisnolarni ishlab chiqish

Obyektga yo’naltirilgan dasturlashda istisnolardan foydalanishga yondashuv ham ajralib turadi. Strukturaviy dasturlashda istisnolarni hal qilishning asosiy strategiyasi funksiyalarning xato kodlarini qaytarishdir. Ular asta-sekin main funktsiyada chiqardi, bu ko'pincha turli xil joylashtirishning shartli bayonotlari ko'rinishidagi xato ishlov beruvchilardan iborat bo'lishi kerak edi. Obyektga yo’naltirilgan dasturlash istisno (exception) tizimidan foydalanishni taklif qiladi. Exception - bu dasturda xatolik yuzaga kelsa, "tashlanishi" mumkin bo'lgan maxsus sinf. O'chirilgan istisno joriy usulning bajarilishini avtomatik ravishda to'xtatadi va qo'ng'iroqlar to'plamini maxsus try-catch bloki bilan bir xil doirada bo'lgunga qadar chiqara boshlaydi.

Masalan bizda quyidagi kod bor:

```cpp
#include <iostream>
#include <exception>
 
// division funtion
double divide(double x, double y)
{
    // check division by zero
    if (y == 0)
        // throw standard exception object
        // this will be thrown only if y == 0
        // divide function stops here if y == 0 and returns nothing, but exception
        throw std::exception();
    // perform division
    return x / y;
}
 
int main()
{
    // try block
    // it tries the operation and immediately jumps to the catch block if any exception occurs
    try
    {
        // trying division by zero
        double result = divide(1, 0);
        // this will never happen because y == 0
        std::cout << result << std::endl;
    }
    // catch block
    catch (const std::exception& err)
    {
        // some exception handling here...
    }
    return 0;
}
```

Biroq, istisno turlarini farqlash yoki istisno obyekti bilan birga qo’shimcha ma’lumotlarni uzatish uchun istisno asosiy sinfga asoslangan maxsus sinflarni yaratish mumkin. Buning uchun siz o'zingizning istisno sinfingizni yaratishingiz va uni `std::exception` dan meros qilib olishingiz mumkin.


# Sinf tuzilishi

C++ tilida sinflar tuzilmalarga o’xshaydi.:

```cpp
class <sinf ismi> 
{
   <dustup modifikatori>:
      <atribut yoki usul>
};
```

Biroq bu yerda maydonlar o'rniga oddiy o'zgaruvchilar sifatida belgilanadigan atributlar. Bundan tashqari, sinf ichida siz usullarni - sinfning barcha atributlariga kirish huquqiga ega funktsiyalarni belgilashingiz mumkin. Usullar va atributlar sinf bilan uzviy bog’liqdir. Kirish modifikatorlari (public, protected и private) sinfning metodlari va atributlariga tashqi kirishni tartibga soladi: xususiy atributlar va usullarga faqat sinf ichidan kirish mumkin, ochiq bo'lganlarga esa har qanday tashqi funksiya yoki boshqa sinfdan kirish mumkin. Xususiy atributlar foydalanuvchidan ichki maydonlarni amalga oshirishni yashirish va xavfsiz umumiy kirish (accessor) va o’zgarishlar (mutator) usullarini ta'minlash imkonini beradi. Odatda `x` xususiy atributiga kirish uchun `getX()` - accessor va `setX()` - mutator umumiy usullari aniqlanadi.

Atributlar yoki usullarga to'g'ridan-to'g'ri kirish uchun struktura maydonlariga kirish sintaksisini takrorlaydigan sintaksis ishlatiladi. Masalan, `myObject.myArgument` `myArgument` obyektidan `myArgument` atributining qiymatini qaytaradi va `myObject.myMethod()` myObject obyektining `myMethod()` usulini bajaradi. Obyektga ko’rsatgichdan foydalanganda, argumentlar va usullarga kirish sintaksisi tuzilmalarga ko'rsatgichlarga o'xshaydi. Masalan, `myObjectPointer->myMethod()` `myObjectPointer` tomonidan ko'rsatilgan obyektning `myMethod()` usulini chaqiradi.

Sinfda shuningdek konstruktorlar va destruktorlar deb ataladigan maxsus usullar aniqlangan bo'lishi mumkin. Ushbu sinf obyekti yaratilganda yoki o'chirilganda ular avtomatik ravishda chaqiriladi. Ularda kerakli maydonlar ajratib ko'rsatiladi va to'ldiriladi yoki aksincha bo’shatiladi.

Konstruktorni e’lon qilish uchun siz qaytarish turisiz sinf nomi bilan usulni ko'rsatishingiz kerak, masalan:

```cpp
Matrix();
```

Destruktor xuddi shunday tarzda e'lon qilinadi, lekin tilda bilan boshlanadi:

```cpp
~Matrix();
```

C++ bir sinf uchun boshqa argumentlar to'plamiga ega bo'lgan bir nechta konstruktorlarni yaratishga imkon beradi, bu sinf obyektlarini yaratishda, yoki o'tkazilgan argumentlarga mos keladigan konstruktor chaqiriladi yoki hech qanday argument berilmagan bo'lsa, boshlang’ich konstruktor:

```cpp
Matrix();               // boshlang’ich konstruktor
Matrix(int n);          // bitta argumentli konstruktor, masalan, n dan n gacha bo’lgan kvadrat matritsani yaratadi
Matrix(int n, int m);   // ikkita argumentli konstruktor, masalan, n ga m matritsa yaratadi
```

Ushbu konstruktorlarni dastur kodida chaqirish:

```cpp
int n = 4, m = 5;
Matrix m1;               // boshlang’ich konstruktor
Matrix m2(n);            // bitta argumentli konstruktor, masalan, n dan n gacha bo’lgan kvadrat matritsani yaratadi
Matrix m3(n, m);         // ikkita argumentli konstruktor, masalan, n ga m matritsa yaratadi
```

Konstruktor metodini Yaratilgan obyektning odatiy usuli sifatida konstruktor usulini qayta chaqirishga urinmaslik kerak. Konstruktorlar faqat yangi obyekt yaratish uchun ishlatiladi.

Obyektga ko’rsatgich yaratish uchun `new` kalit so’zidan foydalaning:

```cpp
Matrix* pM = new Matrix(4,5);
```

E’ibor bering, o’tkaziladigan argumentli qavslar endi o'zgaruvchidan emas, balki sinf nomidan keyin qo'yiladi. `pM` tomonidan ko'rsatilgan obyektni qo'lda o'chirish uchun siz `delete` kalit so'zidan foydalanishingiz mumkin:

```cpp
delete pM;              // bu yerda avtomatik ravishda destruktor chaqiriladi ~Matrix()
```

Agar obyekt new orqali emas, balki oddiy e’lon orqali yaratilgan bo’lsa, destruktor ushbu o’zgaruvchi yaratilgan doiradan chiqqanda chaqiriladi:

```cpp
int main() {
    Matrix m(4);

    // Matritsa ustidan ba’zi operatsiyalar, funksiya va usullarni chaqirish...

    return 0;           // bu yerda destruktor avtomatik ravishda chaqiriladi
}
```

Boshlang’ich konstruktordan tashqari (bo'sh kirish parametrlari to'plamiga ega bo'lgan konstruktor), yana ikkita maxsus konstruktor mavjud: nusxa ko'chirish konstruktori va ko'chirish konstruktori. Nusxa ko'chirish konstruktori bir xil sinfdagi boshqa obyektning barcha atributlarini yangi obyektga nusxalash orqali obyektni ishga tushirish uchun mo'ljallangan. U argument sifatida nusxa ko'chirilayotgan obyektga havolani o'z ichiga oladi. Ko'chirish konstruktori nusxa ko'chirish konstruktori bilan deyarli bir xil ko'rinadi, lekin u ko'chirilayotgan obyektning atributlarini tozalashi kutilmoqda. U parametr sifatida ko'chirilayotgan obyektga qo'sh havolani o'z ichiga oladi. Ko'chirish konstruktori odatda ko'chirilayotgan obyekt tez orada yo'q qilinadigan hollarda bajariladi. Qo'shimcha ma'lumot olish uchun `code-samples` misolga qarang. Nusxa ko’chirish va joylashtirish konstruktorlari quyidagicha ko'rinadi:


```cpp
Matrix (Matrix&);       // nusxalash konstruktori
Matrix (Matrix&&);      // ko’chish konstruktori
```

C++ da, C tilidagi kabi, oddiy kodli fayllardan (.cpp/.cc) tashqari, sarlavhali fayllar (.h) ham mavjud. Sizga shuni eslatib o'tamizki, kompilyator har safar `#include `direktivasiga duch kelganida, sarlavha fayllaridagi kod dastlabki ishlov berish bosqichida avtomatik ravishda manba kodiga ko'chiriladi. Avvalgidek, sarlavha fayllari kodning boshqa bo'limlari tomonidan havola qilinadigan nomlarni e'lon qilish uchun xizmat qiladi. Ushbu kontseptsiyani obyektga yo'naltirilgan paradigmaga o'tkazsak, biz eng keng tarqalgan holatda sinf va uning usullari sarlavha faylida e'lon qilinganligini topamiz. Sarlavha fayllari sinf usullarini yoki alohida funktsiyalarni o'z ichiga olmaydi.

C dan farqli o'laroq, C ++ nomlar maydoni doirasi tushunchasi bilan ham shug'ullanadi. Nomlar maydoni - bu bir-biri bilan bog'langan turli identifikatorlar (turlar, funktsiyalar, o'zgaruvchilar va boshqalar nomlari) aniqlangan maydon. Nomlar bo'shliqlari kodlarni tematik guruhga mantiqiy ravishda guruhlash va turli kontekstual ma'noga ega bo'lgan o'xshash nomlar o'rtasidagi ziddiyatlarni oldini olish uchun ishlatiladi. Buni misol bilan tushunish ancha oson. Misol uchun, ba'zi bir keng qamrovli dasturda daraxtlar uchun ikkita sinfni tavsiflash kerak edi, lekin boshqacha: bir holatda, masalan, sinf o'simliklar kabi daraxtlarning ba'zi botanika xususiyatlarini tavsiflaydi va boshqa holatda, masalan, bu shajara daraxtining ierarxik ko'rinishi. Siz ushbu sinflar uchun turli nomlarni topishingiz mumkinligi aniq, ammo bu shart emas. Kontekstdan ko'rinib turibdiki, sinflar turli xil obyektlarga murojaat qiladi va turli mantiqni amalga oshiradi. `Botany` va `Family` kabi ikkita turli nomlar maydonini kiritish mumkin, ular potentsial ravishda ushbu tushunchalar bilan bog'liq bo'lgan boshqa ko'plab sinflarni o'z ichiga olishi mumkin. Bu shunday ko'rinadi:

```cpp
namespace Botany
{
    class Tree
    {
        // daraxtni o'simlik sifatida ifodalovchi sinf
    }
}
```

```cpp
namespace Family
{
    class Tree
    {
        // shajara daraxtini ifodalovchi sinf
    }
}
```

Endi bu sinflarga tegishli nomlar maydonidan tashqari kirish uchun quyidagi `<Nomlar sohasi nomi>::<nom> `sintaksisidan foydalanish kifoya, masalan: oila daraxtiga kirish uchun biz `Family::Tree` olamiz. Nomlar sohalari murakkab ierarxik tuzilishga ega bo'lishi mumkin, ya'ni ba'zi nomlar sohalari boshqalar ichida aniqlanishi mumkin, so'ngra kirishda ularni joylashtirish tartibida ro'yxatga olish kerak. Mojarolar haqida tashvishlanishingiz shart bo'lmagan hollarda har safar nom maydoni nomini yozishga majbur bo’lmaslik uchun `using` kalit so'zidan foydalanishingiz mumkin.

# Haddan tashqari yuklash

Haddan tashqari yuklash - bir xil nomli, ammo **turli argumentlar** va qaytarish turlari bilan bir nechta turli funktsiyalar va usullarni yaratish qobiliyati. Bu, masalan, universal funktsiyani yaratish zarurati tug'ilganda kerak bo'lishi mumkin. Misol uchun, biz ikkita raqamni qo'shish funktsiyasini yaratishimiz kerak. Uning prototipi quyidagi funktsiya bo'lishi mumkin:

```cpp
int sum(int a, int b);
```

Cda siz har xil turdagi funktsiyalar uchun alohida ilovalar yaratishingiz kerak edi, masalan:

```c
int sum_integer(int a, int b);
double sum_double(double a, double b);
```

Ammo C++ da funksiyalarni haddan tashqari yuklash, ya'ni bir xil nomdagi turli funksiyalarni belgilash mumkin bo’ldi:

```cpp
int sum(int a, int b);              // int turidagi ikkita qiymat o'tkazilganda chaqiriladi
double sum(double a, double b);     // double turidagi ikkita qiymat o'tkazilganda chaqiriladi
```

Bunday holda, ko'proq sonli parametrlar yig'indisi uchun `sum` funksiyasini aniqlash mumkin:

```cpp
int sum(int a, int b, int c);        // int turidagi uchta qiymat o'tkazilganda chaqiriladi
```

# Operatorning haddan tashqari yuklanishi

Obyektga yo'naltirilgan dasturlash operatorni haddan tashqari yuklashni ham amalga oshirdi. Har qanday yangi sinf uchun har qanday standart operator (ikkilik yoki birlik) aniqlanishi mumkin. Masalan, ko'paytirish. Obyektni matritsaga ko'paytirishni `mult(Matrix, Matrix)`, funksiyasi orqali emas, balki `*` operatori orqali aniqlash zarurati tug'ildi. Bu ikkilik operator bo'lib, uning chap va o'ng tomonlari matritsalar bilan ifodalanadi:

```cpp
Matrix operator*(Matrix& x);
``` 

Matritsani raqamga ko’paytirish bir xil operator yordamida ham aniqlanishi mumkin, ammo boshqa imzoga ega usulda, masalan:

```cpp
Matrix operator*(double x);
``` 

Diqqat! Ushbu usul faqat matritsa raqamga ko’paytirilsa (aynan shu tartibda) ishlaydi. Buning sababi shundaki, C++ tilidagi har qanday binar operator birinchi va ikkinchi operand o'rtasida operator sifatida kengaytiriladi va birinchi operandda e'lon qilingan operatorning amalga oshirilishi bajariladi. Double tipidagi o'zgaruvchi matritsani ko'paytirish qanday amalga oshirilishini "bilmaydi". Siz bu muammoni quyidagicha hal qilishingiz mumkin. Ikki argumentli operatordan foydalaning, lekin uni **sinfdan tashqarida** aniqlang. Keyin argumentlar mos keladigan operandlar sifatida ko'rib chiqiladi. Bunday holda, siz haddan tashqari yuklangan operatorning turli xil argumentlar tartibi bilan ikkita nusxasini yaratishingiz kerak bo'ladi, masalan:


```cpp
Matrix operator*(double, Matrix&);
Matrix operator*(Matrix&, double);
```

Bu funksiyalarni Matrix 
Ushbu funktsiyalarni Matrix klassi bilan bog'lash uchun ko'rsatilgan operatorlarni sinf ichida do'stona deb belgilash kifoya:

```cpp
friend Matrix operator*(double, Matrix&);
friend Matrix operator*(Matrix&, double);
```

Operatorlarni haddan tashqari yuklashda siz har doim quyidagi qoidalarni yodda tutishingiz kerak:

- C++ da mavjud bo’lmagan operatorlarni ortiqcha yuklay olmaysiz.
- Operandlarning hech biri foydalanuvchi tipidagi o'zgaruvchi bo'lmasa, operatorlarni ortiqcha yuklab bo’lmaydi;
- operatorlarni haddan tashqari yuklaganda, ushbu operatsiya uchun C++ da belgilangan ustuvorlik saqlanib qoladi va unga ta'sir qilib bo'lmaydi;
- Operator tomonidan taqdim etilgan operandlar sonini o'zgartirib bo’lmaydi.

Bu operatorni haddan tashqari yuklash deb ataladi - taqdim etilgan imzoga qarab bir nechta turli xil operator ilovalaridan foydalanish.


# Beshlik qoidasi 

`Beshlik` qoidasiga ko'ra, agar umumiy holatda, agar obyektni nusxalash, ko’chirish yoki yo’q qilish operatsiyalaridan birini mustaqil ravishda belgilash zarurati tug’ilsa, uning to'g'ri ishlashi uchun siz quyidagilarni amalga oshirishingiz kerak bo’ladi:

- Destruktor 
- Nusxalash konstruktori
- Nusxalash bilan o’zlashtirish operatori 
- Joyini o’zgartirish konstruktori 
- Ko'chirish tayinlash operatori

Beshlik qoidasi misoli:

```cpp
#include <cstring>

class RFive
{
private:
    char* cstring;

public:
    // Initsializatsiya ro’yxati va tanasi bilan konstruktor
    RFive(const char* arg)
    : cstring(new char[std::strlen(arg)+1])
    {
        std::strcpy(cstring, arg);
    }

    // Destruktor
    ~RFive()
    {
        delete[] cstring;
    }

    // Nusxalash konstruktori 
    RFive(const RFive& other)
    {
        cstring = new char[std::strlen(other.cstring) + 1];
        std::strcpy(cstring, other.cstring);
    }

    // Ko’chirish konstruktori, noexcept - standart konteynerlardan foydalanganda optimallashtirish uchun
    RFive(RFive&& other) noexcept 
    {
        cstring = other.cstring;
        other.cstring = nullptr;
    }

    // Nusxalash bilan o’zlashtirish operatori (copy assignment)
    RFive& operator=(const RFive& other) 
    {
        if (this == &other)
            return *this;

        char* tmp_cstring = new char[std::strlen(other.cstring) + 1];
        std::strcpy(tmp_cstring, other.cstring);
        delete[] cstring;
        cstring = tmp_cstring;
        return *this;
    }

    // Joyni almashtirish bilan o’zlashtirish operatori (move assignment)
    RFive& operator=(RFive&& other) noexcept
    {
        if (this == &other)
            return *this;

        delete[] cstring;
        cstring = other.cstring;
        other.cstring = nullptr;
        return *this;
    }

//  Shuningdek, ikkala tayinlash operatorini quyidagi operator bilan almashtirishingiz mumkin
//  RFive& operator=(RFive other)
//  {
//      std::swap(cstring, other.cstring);
//      return *this;
//  }
};
```
