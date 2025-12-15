//*******************************************************************************
//
//  Sun-Mar dynamic QR code generator.
//  Uses a QR code generation library and then outputs a preview to screen
//  and a final version to a BMP file
//
//  M.Janoska
//
//  17 Nov 2025
//
//*******************************************************************************

#include "stdafx.h"

// version information
#define VERSION						"1.0.0"
#define VERSION_DATE				"17 Nov 2025"

// command line arguments
#define PRG_NAME_PARM				0
#define PARM_URL					1
#define PARM_SN						2
#define PARM_PROD					3
#define PARM_SCALE					4

//resource limits
#define MAX_ROW_PIXEL	4000

//forawrd references
static void printQr(const uint8_t qrcode[]);
static void outputQr(const uint8_t qrcode[], int scale) ;

//BMP Header structure
#pragma pack(2)
typedef struct bmpHeader 
	{
	uint16_t id;
	uint32_t fSize;
	uint16_t appSpec1;
	uint16_t appSpec2;
	uint32_t bmOffset;
	} bmpHdrType;

//DIB Header structure
typedef struct dibHeader 
	{
	uint32_t dibSize;
	uint32_t width;
	uint32_t height;
	uint16_t numPlane;
	uint16_t bpp;
	uint32_t compression;
	uint32_t bmDataSize;
	uint32_t resHorz;
	uint32_t resVert;
	uint32_t numPalletColor;
	uint32_t numImportantColor;
	uint32_t redMask;
	uint32_t grnMask;
	uint32_t bluMask;
	uint32_t alphaMask;
	uint32_t colorSpace;
	uint8_t csEnd[36];
	uint32_t redGamma;
	uint32_t grnGamma;
	uint32_t bluGamma;
	} dibHdrType;
	   


//***********************************************************************************
//  
//  function _tmain()
//
//  Main entry point for dynamic generation of Sun-Mar QR codes.
//
//***********************************************************************************
int _tmain(int argc, _TCHAR* argv[])
{
uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
enum qrcodegen_Ecc errCorLvl;
char text[500];
bool ok;
int scale;

// usage message
printf("\n\nSun-Mar Dynamic QR Code Generator  -  Version %s (%s)\n\n",VERSION,VERSION_DATE); 

if(argc!=5)
   {
   printf("Usage\n");
   printf("-----\n");
   printf("%s <web URL> <serial number> <product> <size scale>\n",argv[PRG_NAME_PARM]);
   printf("\n");
   return 0;
   };

// show command line parameters
printf("  Web URL: %s\n",argv[PARM_URL]);
printf("  Serial Number: %s\n",argv[PARM_SN]);
printf("  Product: %s\n",argv[PARM_PROD]);
printf("  Scale: %s\n",argv[PARM_SCALE]);
printf("\n");

sscanf(argv[PARM_SCALE],"%d",&scale);
if(scale<1 || scale>16) scale =1;

//inputs for the QR generation
sprintf(text,"%s?sn=%s&prod=%s&cc=XXXXX",argv[PARM_URL],argv[PARM_SN],argv[PARM_PROD]);         
errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level
	
// generate the QR Code symbol
ok=qrcodegen_encodeText(text,tempBuffer,qrcode,errCorLvl,qrcodegen_VERSION_MIN,qrcodegen_VERSION_MAX,qrcodegen_Mask_AUTO,true);
	
if(ok)
	{
	printf("Successful generating QR code\n\n");
	printQr(qrcode);
	outputQr(qrcode,scale);
	}
else
	printf("Error generating QR code\n\n");

return 0;
}


//*****************************************************************************************
//
//  function: printQr()
//
//  Displays a representation of a QR code on the text monitor. Sometimes this can even
//  be read properly by a smart phone from the screen!
//  
//****************************************************************************************
static void printQr(const uint8_t qrcode[]) 
{
int size;
int border;
int x,y;
uint8_t pix[3]={0xdb,0xdb,00};

size=qrcodegen_getSize(qrcode);
border=4;

for (y=-border;y<size+border;y++) 
	{
	for (x=-border;x<size+border;x++) 
		{
		fputs((qrcodegen_getModule(qrcode,x,y)?(const char *)pix:"  "), stdout);
		}
	fputs("\n",stdout);
	}
fputs("\n",stdout);
}


//*****************************************************************************************
//
//  function: outoutQr()
//
//  Outputs the QR code to a BMP file.
//  
//****************************************************************************************
static void outputQr(const uint8_t qrcode[], int scale) 
{
int size;
int border;
int x,y;
int i,k;
int rowPixel;
int rawPixel;
int rowModule;
int rawModule;
FILE *fout;
bool val;
int ind;
int bmDataSize;
uint32_t row[MAX_ROW_PIXEL];
char fname[100];

bmpHdrType bmpHdr;
dibHdrType dibHdr;

printf("Generate QR Code BMP file\n");
printf("-------------------------\n");

//BMP output file
sprintf(fname,"qr-%d.bmp",scale);
fout=fopen(fname,"wb");
if(fout==NULL)
	{
	printf("Error opening BMP output file\n\n");
	return;
	};

//get qr code size information
size=qrcodegen_getSize(qrcode);
border=4;

//calculate the size of the dm raw pixel data
rowModule=size+2*border;
rawModule=rowModule*rowModule;
rowPixel=rowModule*scale;
rawPixel=rowPixel*rowPixel;
bmDataSize=rawPixel*4;

printf("QR module size=%d\n",size);
printf("Border module size=%d\n",border);
printf("Row module size =%d\n",rowModule);
printf("Total number of modules = %d\n",rawModule);
printf("Scale=%d\n",scale);
printf("Row pixel size=%d\n",rowPixel);
printf("Total pixel size=%d\n",rawPixel);
printf("Bits per pixel=%d\n",32);

printf("\n");
printf("Size of bmp header = %d\n",sizeof(bmpHdrType));
printf("Size of dib header = %d\n",sizeof(dibHdrType));
printf("Size of Bit Map Data = %d\n",bmDataSize);

//see if resource limits exceeded
if(rowPixel>MAX_ROW_PIXEL)
	{
	printf("\nBMP row pixel size too large\n");
	return;
	};

//bmp header values
bmpHdr.id=0x4d42;
bmpHdr.fSize=sizeof(bmpHdrType)+sizeof(dibHdrType)+bmDataSize;
bmpHdr.appSpec1=0x0000;
bmpHdr.appSpec2=0x0000;
bmpHdr.bmOffset=sizeof(bmpHdrType)+sizeof(dibHdrType);

//dib header values
dibHdr.dibSize=108;
dibHdr.width=rowPixel;
dibHdr.height=rowPixel;
dibHdr.numPlane=1;
dibHdr.bpp=32;
dibHdr.compression=3;
dibHdr.bmDataSize=bmDataSize;
dibHdr.resHorz=2835;
dibHdr.resVert=2835;
dibHdr.numPalletColor=0;
dibHdr.numImportantColor=0;
dibHdr.redMask=0x00FF0000;
dibHdr.grnMask=0x0000FF00;
dibHdr.bluMask=0x000000FF;
dibHdr.alphaMask=0xFF000000;
dibHdr.colorSpace=0x57696e20;
for(i=0;i<36;i++) dibHdr.csEnd[i]=0x00;
dibHdr.redGamma=0;
dibHdr.grnGamma=0;
dibHdr.bluGamma=0;

//write the BMP header and the dib header 
fwrite((const void*)&bmpHdr,1,sizeof(bmpHdrType),fout);
fwrite((const void*)&dibHdr,1,sizeof(dibHdrType),fout);

//generate BMP pixels and write each row to output
for (y=size+border-1;y>=-border;y--)
	{
	ind=0;
	for (x=-border;x<size+border;x++)
		{
		val=qrcodegen_getModule(qrcode,x,y);

		for(k=0;k<scale;k++)
			{
			if(val)
				row[ind]=0xff000000;
			else
				row[ind]=0xffffffff;
			ind++;
			};
		};

	for(k=0;k<scale;k++) fwrite((const void*)row,4,ind,fout);
	};

fclose(fout);
printf("\nBMP file generated ok - %s\n",fname);
}

