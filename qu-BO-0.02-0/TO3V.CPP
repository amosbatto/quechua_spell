/*****************************************************************

	to3v.cpp transforma archivos pre-diccionario y affix de
	5 vocales a 3 vocales.

	Autor: Amos B. Batto <abatto@indiana.edu> C. 2004

	Licencia: Puede copiar, cambiar, y distribuir este programa
	de acuedo con el GPL (General Protection License) del
	Free Software Foundation <www.fsf.org>

******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//tama�o maximos de l�nea leido del pre-diccionario y escrito al diccionario
#define MAXLINE 	256

//Si el sistema operativo usa 2 byte caracteres como Unicode 16:
//# define OS_CHAR wchar_t
#define OS_CHAR	char

//Si el archivo pre-diccionario usa 2 byte caracteres:
#define CHAR	wchar_t
//#define CHARchar

//Return codes: valores cuando sale de una funci�n o sale del programa:
#define RET_SUCCESS	0 	//exito
#define RET_HELP	1	//usuario quiere ayuda
#define ERR_OPEN_FILE	4	//no puede abrir un archivo
#define ERR_WR_FILE	6	//error escribiendo a un archivo
#define ERR_RD_FILE	7	//error leyendo de un archivo

//Los flags por omisi�n:  (Puede redefinirlos con argumentos pasados)
CHAR IspFlg = '/';	//flag de Ispell
CHAR CmtFlg = '#';  //flag para comentarios

OS_CHAR * SInFile = NULL;	//nombre de archivo para leer (input)
OS_CHAR * SOutFile = NULL;	//nombre de archivo transformado (output)
FILE * PInFile = NULL;		//file pointer para el pre-diccionario
FILE * POutFile = NULL;		//file pointer para el diccionario

//+1 para el NULL al fin de string  
CHAR SLine[MAXLINE+1];		//l�nea de texto le�do del archivo PInFile
CHAR SNewLine[MAXLINE+1];	//l�nea de texto escrito al archivo POutFile

int CntSLine = 0;	//contar car�cters en SLine[]
int LenLine = 0;	//tama�o de SLine en car�cteres
long LCntE = 0;		//contar n�mero de cambios de vocales de e -> i
long LCntO = 0;		//contar n�mero de cambios de vocales de o -> u
long LCntLines = 0;	//contar n�mero de lineas escrito al archivo POutFile

int main(int argc, OS_CHAR ** argv)
{
	//pone todos los strings a vacio.
	SLine[0]=SNewLine[0]=NULL;

	if (argc >= 2 && !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help") ||
		!strcmp(argv[1],"/?"))
	{
		fprintf(stdout,
		"to3v copia texto de archivo1 a archivo2 y cambia las vocales e -> i y o -> u\n"
		"formato: TO3V achivo1 archivo2\n");
		return RET_HELP;
	}

	if (argc >= 2)
		SInFile = argv[1];
	else
		SInFile = "qu-BO-pd.txt";

	if (argc >= 3)
		SOutFile = argv[2];
	else
		SOutFile = "qu-BO-3p.txt";

	//abrir en modo texto solo para leer
     	//si hay error al abrir el pre-diccionario
	if ((PInFile = fopen(SInFile, "rt")) == NULL)
	{
		fprintf(stderr,"Error: No puede abrir archivo \"%s\" para leer.\n",
			SInFile);
		return ERR_OPEN_FILE;
	}

	//Aviso: esto borra todo el archivo y re-escribir de nuevo.
	//si hay error al abrir el diccionario
	if ((POutFile = fopen(SOutFile, "wt")) == NULL)
	{
		fprintf(stderr,"Error: No puede abrir archivo \"%s\" para escribir.\n",
			SOutFile);
		return ERR_OPEN_FILE;
	}
	
     
	//pasar por el archivo pre-diccionario, leyendo una l�nea por cada ciclo
	while (!feof(PInFile))
	{
		//si hay error al leer (no haga nada si el fin del archivo)
		if (fgets(SLine, MAXLINE, PInFile) == NULL && ferror(PInFile))
		{
	  		fprintf(POutFile,"Error leyendo archivo \"%s\"\n",SInFile);

			fclose(PInFile);
			fclose(POutFile);
			return ERR_RD_FILE;
		}

		LCntLines++;	//incrementar contador de lineas
		LenLine = strlen(SLine);

		//Va hasta que encuentre un commentario, un ispell flag, o el fin de l�nea 
		for (CntSLine = 0; CntSLine < LenLine && SLine[CntSLine] != CmtFlg
				&& SLine[CntSLine] != IspFlg; CntSLine++)
		{
			switch (SLine[CntSLine]) {
				case 'e': SNewLine[CntSLine] = 'i'; LCntE++; break;
				case '�': SNewLine[CntSLine] = '�'; LCntE++; break;
				case 'E': SNewLine[CntSLine] = 'I'; LCntE++; break;
				case '�': SNewLine[CntSLine] = '�'; LCntE++; break;
				case 'o': SNewLine[CntSLine] = 'u'; LCntO++; break;
				case '�': SNewLine[CntSLine] = '�'; LCntO++; break;
				case 'O': SNewLine[CntSLine] = 'U'; LCntO++; break;
				case '�': SNewLine[CntSLine] = '�'; LCntO++; break;
				default: SNewLine[CntSLine] = SLine[CntSLine];
                        }
		}		

		//copie lo dem�s de la l�nea despu�s de un commentario o un ispell flag
		for (; CntSLine < LenLine; CntSLine++)
                	SNewLine[CntSLine] = SLine[CntSLine];

		//termina la l�nea con NULL c�racter
                SNewLine[CntSLine] = NULL;
			 
		//Si hay un error escribiendo SNewLine en el archivo POutFile
		if (fputs(SNewLine, POutFile) == EOF)
		{
			fprintf(stderr,"Error escribiendo al archivo \"%s\"\n",SInFile);
			fclose(PInFile);
			fclose(POutFile);
			return ERR_WR_FILE;
		}
	}//llega al fin del archivo PInFile
 
	fprintf(stdout,	"%s l�neas escrito en el archivo \"%s\"\n",
		ltoa(LCntLines, SLine, 10), SOutFile);
	fprintf(stdout, "N�mero de E -> I: %s\nN�mero de O -> U: %s\n",
		ltoa(LCntE, SLine, 10), ltoa(LCntO, SNewLine, 10));

	fclose(PInFile);
	fclose(POutFile);

	return RET_SUCCESS;
}

