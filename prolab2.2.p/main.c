#include <stddef.h> // NULL
#include <stdint.h>
#include <stdlib.h> // calloc
#include <string.h> // memset
#include <stdio.h>

#define OFFSETMASK(x) (1 << x)
#define LENGTHMASK(x) (1 << x)
#define OFFSETLENGTH(x,y) (x - y)

struct token {//3 yerine 2 olması boyutu azaltırken token oluşturma döngüm benzer grupları atlayarak sıkıştırma yapıyor
    uint8_t offset_len;//1 byte tüm platformda bu syde değişkenlik göstermez ve boyut değişmez
    char c;
};

struct token *encode(char *text, int limit, int *numTokens,int kontrol)
{
    // cap (kapasite) hafýzada kaç tokenlik yer ayýrdýðýmýz.
    int cap = 1 << 3;//8 bit harfler de

    // kaç token oluþturduðumuz.
    int _numTokens = 0;

    // oluþturulacak token
    struct token t;

    // lookahead ve search buffer'larýnýn baþlangýç pozisyonlarý
    char *lookahead, *search;

    // tokenler için yer ayýr.
    struct token *encoded = malloc(cap * sizeof(struct token));

    // token oluþturma döngüsü
    for (lookahead = text; lookahead < text + limit; lookahead++)
    {
        // search buffer'ý lookahead buffer'ýn 31 (OFFSETMASK) karakter
        // gerisine koyuyoruz.
        if(kontrol==0)
        search = lookahead - OFFSETMASK(3);
        else
        search = lookahead - OFFSETMASK(5);
        // search buffer'ýn metnin dýþýna çýkmasýna engel ol.
        if (search < text)
            search = text;

        // search bufferda bulunan en uzun eþleþmenin
        // boyutu
        int max_len = 0;
        int len;
        // search bufferda bulunan en uzun eþleþmenin
        // pozisyonu
        char *max_match = lookahead;

        // search buffer içinde arama yap.
        for (; search < lookahead; search++)
        {
            if(kontrol==0)
                len = prefix_match_length(search, lookahead, LENGTHMASK(5));
            else
                len = prefix_match_length(search, lookahead, (LENGTHMASK(8)+2));
            if (len > max_len)
            {
                max_len = len;
                max_match = search;
            }

        }

        /*
        * ! ÖNEMLÝ !
        * Eðer eþleþmenin içine metnin son karakteri de dahil olmuþsa,
        * tokenin içine bir karakter koyabilmek için, eþleþmeyi kýsaltmamýz
        * gerekiyor.
        */
        if (lookahead + max_len >= text + limit)
        {
            max_len = text + limit - lookahead - 1;
        }


        // bulunan eþleþmeye göre token oluþtur.
        t.offset_len=OFFSETLENGTH(lookahead,max_match);
        lookahead += max_len;
        t.c = *lookahead;

        // gerekirse, hafýzada yer aç
        if (_numTokens + 1 > cap)
        {
            cap = cap << 1;//8 Dİ 16 YAPACAK ...
            encoded = realloc(encoded, cap * sizeof(struct token));
        }
        //printf("<%d,%d,%c>,",t.offset_len,max_len,t.c);
        // oluþturulan tokeni, array'e kaydet.
        encoded[_numTokens++] = t;
    }
    if (numTokens)
        *numTokens = _numTokens;//fwrite ta kullanmak için lazım

    return encoded;
}
inline int prefix_match_length(char *s1, char *s2, int limit)
{
    int len = 0;
    while (*s1++ == *s2++ && len < limit)
        len++;

    return len;
}

#ifdef _MSC_VER
__pragma(pack(push, 1))
struct huffmancode {
	uint16_t code;
	uint8_t len;
};
__pragma(pack(pop))
#elif defined(__GNUC__)
struct __attribute__((packed)) huffmancode {
	uint16_t code;
	uint8_t len;
};
#endif

// her bir code için uzunluk ve kod değerlerini
// tuttuğumuz array
struct huffmancode tree[0X100];

// her uzunlukta kaç adet kod
// olduğunu saymak için
uint8_t bl_count[0x10];

// her uzunluktaki kodlama için
// atanacak kodu tutar
uint16_t next_code[0x10];


typedef struct _huffman//harfler ve frekansları
{
	char c;
	int freq;
	struct _huffman *left;
	struct _huffman *right;
} HUFFMANTREE;

typedef struct _huffman_array {
	int cap;
	int size;
	HUFFMANTREE **items;
} HUFFMANARRAY;

HUFFMANTREE *huffmantree_new(char c, int freq)
{
	HUFFMANTREE *t = malloc(sizeof(HUFFMANTREE));
	t->c = c;
	t->freq = freq;
	t->left = NULL;
	t->right = NULL;
	return t;
}

/*
* Selection sort, büyükten küçüğe
*/
void huffman_array_sort(HUFFMANARRAY *arr)
{
	int i, k;
	int max_index, max_value;

	for (i = 0; i < arr->size - 1; i++)
	{
		max_index = i;
		max_value = arr->items[i]->freq;

		for (k = i + 1; k < arr->size; k++)
		{
			if (arr->items[k]->freq > max_value)
			{
				max_value = arr->items[k]->freq;
				max_index = k;
			}
		}

		if (i != max_index)
		{
			HUFFMANTREE *_tmp = arr->items[i];
			arr->items[i] = arr->items[max_index];
			arr->items[max_index] = _tmp;
		}

	}
}

HUFFMANTREE *huffman_array_pop(HUFFMANARRAY *arr)
{
	return arr->items[--arr->size];//ağaç bütünlüğü
}

void *huffman_array_add(HUFFMANARRAY *arr, HUFFMANTREE *t)
{
	if (arr->size + 1 == arr->cap)//0000 0000 lı gösterim için bloklar halinde ve t1 t2 birleşimi için boyut artıyor t3
	{
		arr->cap *= 2;
		arr->items = realloc(arr->items, arr->cap * sizeof(HUFFMANTREE *));
	}

	arr->items[arr->size++] = t;
}

HUFFMANARRAY *huffman_array_new()
{
	HUFFMANARRAY *arr = malloc(sizeof(HUFFMANARRAY));
	arr->cap = 8;
	arr->size = 0;
	arr->items = malloc(arr->cap * sizeof(HUFFMANTREE *));
	return arr;
}

void load_canonical_codes_from_tree(HUFFMANTREE *t, int length)//temel ağaca tamamını ekleme işlemi esasen
{
	if (!t)
		return;

	if (t->c != 0)
	{
		tree[t->c].len = length;
	}

	load_canonical_codes_from_tree(t->left, length + 1);
	load_canonical_codes_from_tree(t->right, length + 1);

}

char *code_to_binary(struct huffmancode code)
{
	char *b = malloc(code.len + 1); // +1 null
	int i;

	for (i = 0; i < code.len; i++)
	{
		b[i] = code.code & (1 << (code.len - i - 1)) ? '1' : '0';
	}

	b[code.len] = 0;
	return b;
}
char *file_read(FILE *f, int *size)
{
    char *content;
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    content = malloc(*size);
    fseek(f, 0, SEEK_SET);
    fread(content, 1, *size, f);
    return content;
}
int main(void)
{
    FILE *f;
    unsigned long frequencies[0xFF];
    int metin_boyutu , token_sayisi,k=0;
    char *kaynak_metin;
    struct token *encoded_metin;
    char *pcTemp;
	int i,j;
    int lz77,deflate;

    f = fopen("metin.txt", "rb");
    kaynak_metin = file_read(f, &metin_boyutu);
    fclose(f);

    encoded_metin = encode(kaynak_metin, metin_boyutu, &token_sayisi,k);

    f = fopen("LZ77.txt", "wb");
    fwrite(encoded_metin, sizeof(struct token), token_sayisi, f);
    fclose(f);

    lz77=token_sayisi * sizeof(struct token);
    printf("LZ77 - Encode Boyutu: %d\n", lz77);
    k=1;
    //HUFFMAN
    HUFFMANARRAY *arr = huffman_array_new();

	memset(&frequencies[0], 0, sizeof(frequencies));

	for (pcTemp = &kaynak_metin[0]; *pcTemp != 0; pcTemp++)
	{
		frequencies[(int)*pcTemp]++;//a harfi kaç defa gelirse frekans o kadar artar,int pctemp olma nedeni de her harfin 2lileri farklı

	}

	for (i = 0; i < 255; i++)//255 ,1 byte char ın yani metindeki tüm charların örnek a nın frekansını yolluyor
	{
		if (frequencies[i] > 0)
		{
			huffman_array_add(arr, huffmantree_new(i, frequencies[i]));
		}
	}

	while (arr->size > 1)//2 ağacı bir büyüğe bağlıyoruz hufman yani
	{
		huffman_array_sort(arr);
		HUFFMANTREE *t1 = huffman_array_pop(arr);
		HUFFMANTREE *t2 = huffman_array_pop(arr);
		HUFFMANTREE *t3 = calloc(1, sizeof(HUFFMANTREE));
		t3->left = t1;
		t3->right = t2;
		t3->freq = t1->freq + t2->freq;
		huffman_array_add(arr, t3);

	}

	memset(tree, 0, sizeof(tree));
	memset(bl_count, 0, sizeof(bl_count));
	memset(next_code, 0, sizeof(next_code));

	load_canonical_codes_from_tree(huffman_array_pop(arr), 0);
	for (i = 0; i < 256; i++)
	{
		bl_count[tree[i].len]++;
	}

	int code = 0;
	bl_count[0] = 0;

	for (i = 1; i < 0x10; i++)
	{
		code = (code + bl_count[i - 1]) << 1;
		next_code[i] = code;
	}

	for (i = 0; i < 0x100; i++)
	{
		int len = tree[i].len;
		if (len)
		{
			tree[i].code = next_code[len];
			next_code[len]++;
		}
	}

    f=fopen("deflate.txt","w");
    fclose(f);
    f=fopen("deflate.txt","a");
    for (i=0; i<metin_boyutu;i++)
    {

        for (j = 0; j < 0x100; j++)
        {
            int len = tree[j].len;
            if (len)
            {
                if(kaynak_metin[i]==j)
                {
                    fprintf(f,code_to_binary(tree[j]));
                }
            }
        }
    }
    fclose(f);

    f = fopen("deflate.txt", "rb");

        kaynak_metin = file_read(f, &metin_boyutu);
        fclose(f);

    encoded_metin = encode(kaynak_metin, metin_boyutu, &token_sayisi,k);
    f = fopen("deflate.txt", "wb");

        fwrite(encoded_metin, sizeof(struct token), token_sayisi, f);
        fclose(f);

    deflate=token_sayisi * sizeof(struct token);
    printf("\nDEFLATE - Encode Boyutu: %d\n",deflate);

    if(deflate>lz77)
    {
        printf("\nLZ77 algoritmasi kullanimi tavsiye edilir.\n");
    }
    else if(deflate<lz77)
    {
        printf("\nDeflate algoritmasi kullanimi tavsiye edilir.\n");
    }
    else if(deflate==lz77)
    {
        printf("\nHer iki algoritma da kullanilabilir.\n");
    }
    return 0;
}
