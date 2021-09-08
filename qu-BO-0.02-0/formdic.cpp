/*****************************************************************

	formdic.c transforma un archivo diccionario para usar con
	buildhash que crear archivos Ispell hash.

	Autor: Amos B. Batto <abatto@indiana.edu> C. 2004

	Licencia: Puede copiar, cambiar, y distribuir este programa
	de acuedo con el GPL (General Protection License) del
	Free Software Foundation <www.fsf.org>

******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//tamaño maximos de línea leido del pre-diccionario y escrito al diccionario
#define MAXLINE 	256 
#define MAXAFFIX	50	//tamaño maximo de los afijos
#define MAXFILENAME	33	//tamaño maximo de nombres de archivos

//Si el sistema operativo usa 2 byte caracteres como Unicode 16:
//# define OS_CHAR wchar_t
#define OS_CHAR	char

//Si el archivo pre-diccionario usa 2 byte caracteres:
#define CHAR	wchar_t
//#define CHARchar

//recibir mensajes de errores al procesar el pre-diccionario:
#define ERR_MSG_ON		1	//recibir mensajes (valor por omisión)
#define ERR_MSG_OFF		0	//no recibir mensajes

//ignorar errores de sintaxis en el pre-diccionario y procesar lo demás
//de la línea:
#define NO_IGNORE_ERR_SYNTAX	1	//no ignorar errores (valor por omisión)		
#define IGNORE_ERR_SYNTAX	0    //ignorar errores

//dirigir todos los mensajes de error al archivo:
#define MSG_TO_FILE		1	 //Mensajes va al archivo
#define NO_MSG_TO_FILE	0     //Valor por omisión. Mensajes va al stderr-la pantalla.

//Return codes: valores cuando sale de una función o sale del programa:
#define RET_SUCCESS		0 	//exito
#define RET_HELP		1	//ayuda	
#define ERR_NO_DIC		2	//no entrega un nombre para el diccionario
#define ERR_NO_PREDIC	3	//no entrega un nombre para el prediccionario
#define ERR_OPEN_DIC	4	//no puede abrir el diccionario
#define ERR_OPEN_PREDIC	5	//no puede abrir el prediccionario
#define ERR_WR_DIC		6	//error escribiendo al diccionario
#define ERR_RD_PREDIC	7	//error leyendo del prediccionario
#define ERR_SYNTAX		8	//error de sintaxis en el pre-diccionario
#define ERR_OPEN_MSG	9	//no puede abrir el archivo para mensajes de pantalla
#define ERR_PARAMETER	10	//error en un parámetro pasado por el usuario.

//Los flags por omisión:  (Puede redefinirlos con argumentos pasados)
CHAR SpcFlg = '<';  //afijo specificación (donde pegar el afijo a la raíz);
CHAR IspFlg = '/';	//flag de Ispell
CHAR IfxFlg = '&';	//flag para insertar infijos en la raiz
CHAR SfxFlg = '$';	//flag para pegar sufijos al fin de la raiz
CHAR RplFlg = '%';	//flag para re-emplazar caracteres en la raiz
CHAR CmtFlg = '#';  //flag para comentarios

//Como va a tratar con errores en el pre-diccionario:
//Asignar valores por omisión:
int ErrMsg = ERR_MSG_ON;
int ErrSyntax = NO_IGNORE_ERR_SYNTAX;
int MsgOutFile = NO_MSG_TO_FILE;
OS_CHAR SMsgOutFile[MAXFILENAME];  //por omisión: "fdic-msg.txt".
FILE * PMsgOutFile = stdout;

OS_CHAR * SInFile = NULL;	//nombre de archivo pre-diccionario (input)
OS_CHAR * SOutFile = NULL;	//nombre de archivo diccionario (output)
FILE * PInFile = NULL;		//file pointer para el pre-diccionario
FILE * POutFile = NULL;		//file pointer para el diccionario

int CntSLine=0;		//contar carácters en SLine[]
int LenLine=0;			//tamaño de SLine en carácteres
long LCntWords = 0;		//contar número total de entradas en el diccionario
long LCntNewWords = 0;	//contar número de entradas nuevas de raíz + afijo
long LCntLines = 0;		//contar número de lineas en el pre-diccionario

//+1 para el NULL al fin de string:     
CHAR SRoot[MAXLINE+1];		//raíz que va recibir el afijo
CHAR	SIspellFlags[MAXLINE+1];	//Ispell flags para el raíz
CHAR	SLine[MAXLINE+1];		//línea de texto leído del archivo pre-diccionario
CHAR	SAffix[MAXAFFIX+1];		//afijo que va a pegar al raíz
CHAR	SNewWord[MAXLINE+1];	//palabra nueva formado de raíz + afijo


//redefinir ANSI función isspace() porque piense que 'ñ' es espacio (en Borland Turbo C++ 3.1)
int isSpace(CHAR ch);

//vuelve un número no cero si es un flag (SfxFlg, IfxFlg, IFlg, o SpcFlg)
int isFlag(CHAR ch);

//vuelve el array posición de la última aparición de string sFind dentro de sInStr
int strStrLast(CHAR * sInStr, CHAR * sFind);

//mensaje de ayuda
void helpMsg(OS_CHAR * str);

//quita todo tipo de espacio del comienzo y fin del frase
CHAR * trim(CHAR * str);

//quita todo tipo de espacio del comienzo y fin del frase y quita los comentarios
CHAR * trim(CHAR * str, CHAR cComment);

//quita los flags de un string
CHAR * stripFlags(CHAR * str);

//quita todo tipo de espacio del frase
CHAR * stripSpace(CHAR * str);

//inserta una frase adentro de otra
CHAR * strIns(CHAR * dest, const CHAR * src, int nStart, int nIns, int nReplace);

//saca los afijos. Los pega a la raiz y escribe al diccionario.
int affixes(void);

//abre los archivos y informar al SIspellFlags si hay errores
int openFiles(void);

//saca los parametros pasado por el usuario a este programa
int getArgs(int argC, OS_CHAR ** argV);

//verificar que sirve un parametro que fue pasado por el usuario
int checkParam(CHAR * sParam, CHAR * sCheck, int maxLen, int isBad,
	CHAR * errorMsg, CHAR * sBadArg);


int main(int argc, OS_CHAR ** argv)
{
	int cnt=0;	//contador temporal de posicion en strings

	//pone todos los strings a vacio.
	SRoot[0]=SIspellFlags[0]=SLine[0]=SAffix[0]=SNewWord[0]=NULL;

	//printf("strStrLast(\"billbullybilly\",\"bi\") = %d\n",
	//	strStrLast("billbullybilly", "bull"));

	//printf("strStrLast(\"\", \"bill\") = %d\n", strStrLast("", "bill"));

	//printf("strStrLast(\"gpñ\",\"gpñ\") = %d\n",strStrLast("gpñ", "gpñ"));

	//printf("strStrLast(\"reishallowhallow\", \"shallow\") = %d\n",
	  //	strStrLast("reishallowhallow", "shallow"));

	//printf("strIns(, ABCDEFG, 3, 3, 8) =\"%s\"\n",
	//	strIns("", "ABCDEFG", 3, 3, 8));

	//printf("strIns(ABCDEFGHIJ, 1234567, 0, 7, 2) =\"%s\"\n",
	//	strIns("ABCDEFGHIJ","1234567", 0, 7, 2));

	getArgs(argc, argv);

	openFiles();
	
	printf("SpcFlg = \'%c\'\n"
		"IspFlg = \'%c\'\n"
		"IfxFlg = \'%c\'\n"	
		"SfxFlg = \'%c\'\n"
		"RplFlg = \'%c\'\n"
		"CmtFlg = \'%c\'\n", SpcFlg, IspFlg,IfxFlg,SfxFlg,RplFlg,CmtFlg);

	printf("ErrMsg = %d\n"
		"ErrSyntax = %d\n"
		"MsgOutFile = %d\n"
		"SMsgOutFile = %s\n", ErrMsg, ErrSyntax, MsgOutFile, SMsgOutFile);



     //pasar por el archivo pre-diccionario, leyendo una línea por cada ciclo
	while (!feof(PInFile))
	{
		//si hay error al leer (no haga nada si el fin del archivo)
		if (fgets(SLine, MAXLINE, PInFile) == NULL && ferror(PInFile))
		{
          	if (ErrMsg)
				fprintf(PMsgOutFile,"Error leyendo archivo \"%s\"\n",SInFile);

			fclose(PInFile);
			fclose(POutFile);

			if (MsgOutFile)
				fclose(PMsgOutFile);

               return ERR_RD_PREDIC;
		}

		LCntLines++;	//incrementar contador de lineas

		//quita espacios de comienzo y fin de línea y los comentarios (#)
		trim(SLine, CmtFlg);

		if (strlen(SLine) == 0) //no inserta líneas vacias
			continue;

		CntSLine = 0;
		LenLine = strlen(SLine);

		//Si es una nueva entrada.  Líneas que comienza con un flag (excepto
		//de IFlg) son siguiendo de la entrada de la línea previa.
		if (!isFlag(SLine[0]))
		{

			//saca la raíz. Va hasta encuentra un flag o el fin de línea 
			while ( CntSLine < LenLine && !isFlag(SLine[CntSLine]) )
			{
				SRoot[CntSLine] = SLine[CntSLine];
				CntSLine++;
			}		

               //No sé porque el siguiente linea no sirve en Borland Turbo C++ 3.1
			//SRoot[ CntSLine > 0 ? 0 : CntSLine - 1 ] = '\0';

			SRoot[CntSLine] = NULL;
			trim(stripFlags(SRoot));	//quita espacios al fin del frase
			cnt = 0;

			if (SLine[CntSLine] == IspFlg) //si hay ispell flags
			{
				//saca ispell flags
				while (CntSLine < LenLine && !(isFlag(SLine[CntSLine]) &&
						SLine[CntSLine] != IspFlg))
					SIspellFlags[cnt++] = SLine[CntSLine++];
			}

			//termina SIspellFlags string.
			//sí no habia Ispell flags, string es vacio.
			SIspellFlags[cnt]=NULL;

			//si SRoot es vacio, no inserta nada, y va a línea próxima
			if (strlen(SRoot) == 0)
				break;

			stripSpace(SIspellFlags); //quita los espacios de los flags
			 
			//Si hay error escribiendo raíz, Ispell flags, o return en el diccionario
			if (fputs(SRoot,POutFile) == EOF || fputs(SIspellFlags,POutFile) == EOF ||
					fputc('\n',POutFile) == EOF)
			{
               	if (ErrMsg)
					fprintf(PMsgOutFile,"Error escribiendo al archivo \"%s\"\n",SInFile);
				fclose(PInFile);
				fclose(POutFile);
				if (MsgOutFile)
					fclose(PMsgOutFile);

				return ERR_WR_DIC;
			}

			if (strlen(SRoot) != 0)	//Si escribió una entrada nueva en el diccionario,
				LCntWords++;  		//incrementar contador

			//pasar por lo demás de la línea y añadir los afijos a la raíz
			affixes();
			
		}
		else //si no es entrada nueva, pero siguiendo con una entrada de línea previa
		{
			affixes();
		}
	}//llega al fin del archivo pre-diccionario
	if (ErrMsg)
     {
		fprintf(PMsgOutFile, "Diccionario creado: %s\n", SOutFile);
		fprintf(PMsgOutFile, "Número total de entradas: %s\n",
			ltoa(LCntWords, SLine, 10));
		fprintf(PMsgOutFile, "Número de entradas formado de raíz + afijo: %s\n",
			ltoa(LCntNewWords,SLine,10));
	}

	fclose(PInFile);
	fclose(POutFile);

	if (MsgOutFile)
		fclose(PMsgOutFile);

	return RET_SUCCESS;
}

//vuelve un número no cero si es un flag (RplFlg, SfxFlg,IfxFlg,IFlg, o SpcFlg)
int isFlag(CHAR ch)
{
	if (ch == SfxFlg || ch == IfxFlg || ch == IspFlg || ch == SpcFlg
			|| ch == RplFlg)
		return (int) ch;
	else
		return 0;
}



//quita los flags de un string
CHAR * stripFlags(CHAR * str)
{
	long cntStr = 0;
	long cntRetStr = 0;
	long lenStr=strlen(str);
	CHAR retStr[MAXLINE];

	for (; lenStr > cntStr; cntStr++)
		if (str[cntStr] != IfxFlg && str[cntStr] != SfxFlg &&
			str[cntStr] != SpcFlg && str[cntStr] != IspFlg)
			retStr[cntRetStr++]=str[cntStr];
     	
	retStr[cntRetStr]= NULL; //inserta NULL carácter para terminar la frase

     //copia de retStr a str y vuelve el pointer de str
	return strcpy(str, retStr);
}

//quita todo tipo de espacio del comienzo y fin del frase
CHAR * trim(CHAR * str)
{
	return trim(str, NULL);
}


//Quita todo tipo de espacio del comienzo y fin del string str
//y quita los comentarios que sigue el carácter cComment.
//Pone cComment a NULL, si no quiere quitar commentarios.
CHAR * trim(CHAR * str, CHAR cComment)
{
	long cntStr;
	long cntTempStr = 0;
	long lenStr = strlen(str);
	CHAR tempStr[MAXLINE];

	if (cComment != NULL)
     {
		//busca para los commentarios, y quita lo demás del string
		for (cntStr = 0; lenStr > cntStr; cntStr++)
			if (str[cntStr] == cComment)
				str[cntStr] = NULL; //reemplaza # con NULL
	} 

     lenStr = strlen(str);  //repone el tamaño del string

	//busca el comienzo de frase (primer carácter que no es espacio)
	for (cntStr = 0; lenStr > cntStr && isSpace(str[cntStr]); )
		cntStr++;

     //copia lo demás del frase 
     while (lenStr > cntStr)
		tempStr[cntTempStr++]=str[cntStr++];

	tempStr[cntTempStr] = NULL; //termina el string

     //quita el espacio al fin del frase
	for (cntTempStr--; cntTempStr >= 0 && isSpace(tempStr[cntTempStr]);
				cntTempStr--)		
     	tempStr[cntTempStr]=NULL;

	//Copia de tempStr a str y vuelve el pointer de str.
     return strcpy(str, tempStr);
}

//En Turbo Borland C++3.1, isspace() no sirve para letras como 'ñ'
//Por eso, hay que redefinir isspace()
int isSpace(CHAR ch)
{
	//que es el código de un form feed? '\f' || '\p'?
	if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ||
			ch == '\v' || ch == '\t')
		return ch;
	else
		return 0;
}	

//quita todo tipo de espacio del frase
CHAR * stripSpace(CHAR * str)
{
	long cntStr = 0;
	long cntRetStr = 0;
	long lenStr=strlen(str);
	CHAR retStr[MAXLINE];

	for (; lenStr > cntStr; cntStr++)
		if (!isSpace(str[cntStr]))	//Si es espacio, no copia el carácter
			retStr[cntRetStr++]=str[cntStr];
     	
	retStr[cntRetStr]= NULL; //inserta NULL carácter para terminar la frase

     //copia de retStr a str y vuelve el pointer de str
	return strcpy(str, retStr);
}

//CHAR * strIns(CHAR * dest, const CHAR * src, int nStart, int nIns, int nReplace)
//Inserta string src dentro de string dest. dest y src debe terminar en NULL.
//Inserta nIns número de carácteres de src dentro de string dest,
//empiezando en posición nStart. Si llega al NULL en src antes de
//insertar nIns número de carácteres, para.
//Si nStart es un número negativo, inserta en el fin de dest. 
//Si nIns es un número negativo, inserta hasta el NULL en src.
//nReplace es el número de caracteres in dest que va a re-emplazar.
//La función pone NULL al fin de dest string y vuelve pointer de dest.
CHAR * strIns(CHAR * dest, const CHAR * src, int nStart, int nIns, int nReplace)
{
	CHAR tempStr[MAXLINE];
	int cntTemp,	//contar posición en tempStr
		cnt, 	//contar posición en src y dest
          destLen;	//tamaño de dest

     destLen = strlen(dest);

	//si nStart es negativo o más grande que dest, inserta al fin de dest 
	if (nStart < 0 || nStart >= destLen)
		nStart = destLen > 0 ? destLen : 0;


	//si nIns es negativo o línea sera demasiado grande,
	//pone el maximo tamaño a MAXLINE
	nIns = (nIns < 0 || nIns + nStart >= MAXLINE - 1) ? MAXLINE - nStart - 1 : nIns;

	//si nReplace es demasiado grande
	if (nStart + nReplace > destLen)
     	nReplace = destLen - nStart;

     //copia de dest a tempStr hasta nStart caracter
	for (cntTemp = 0; cntTemp < nStart; cntTemp++)
		tempStr[cntTemp]=dest[cntTemp];

     //inserta de src a tempStr por nIns characteres o hasta llega al NULL
	for (cnt = 0; cnt < nIns && src[cnt] != NULL; )
		tempStr[cntTemp++]=src[cnt++];

	//copia lo demás de dest a tempStr
	for (cnt = nStart + nReplace; (cntTemp < MAXLINE - 1) && dest[cnt] != NULL; )
		tempStr[cntTemp++]=dest[cnt++];

	tempStr[cntTemp]=NULL; //terminar el string

	return strcpy(dest,tempStr);
}


void helpMsg(OS_CHAR * str)
{
	fprintf(PMsgOutFile, str);
	fprintf(PMsgOutFile,"\n"
		"Formato:\n"
		"FORMDIC [-h][-f][-s][-i][-r][-c][-m][-o][-e] pre-diccionario diccionario\n"
		" -h,--help,/?          Ayuda.                                           \n"
		" -f,--flag=C           Flag carácter de Ispell. Por omisión, -f=/       \n"
		" -a,--afijo=C          Flag carácter de afijo specificación. Por omisión, -a=<\n"
		" -s,--suffix=C         Flag carácter de sufijos. Por omisión, -s=$      \n"
		" -i,--infix=C          Flag carácter de infijos. Por omisión, -i=&      \n"
		" -r,--replace=C        Flag carácter para re-emplazar. Por omisión, -r=%\n"
		" -c,--comment=C        Flag carácter de comentarios. Por omisión, -c=#  \n"
		" -m,--messages=yes|no  Mostra mensajes y errores. Por omision, -m=yes   \n"
		" -o,--output[=archivo] Redirigir mensajes de pantalla al archivo.       \n"
		"                       Sin specificar el archivo, mensajes va a \"fdic_msg.txt\"\n" 
		" -e,--errors=yes|no    Procesar entradas con errores de sintaxis en el  \n"
		"                       pre-diccionario. Por omisión, -e=no              \n"
		" pre-diccionario       Nombre del archivo pre-diccionario para procesar.\n"
		" diccionario           Nombre del archivo donde escribe el diccionario Ispell.\n"
		"Ej: formdic -f=@ -c=/ -o -e=yes qu-BO.predict qu-BO.dict                \n");
	return;
}

//pasa por todo la línea sacando afijos. Pega los afijos a la raíz
//y escribe la palabra nueva en el archivo diccionario.
//Vuelve no cero si hay error.
int affixes(void)
{
	int cnt = 0;	//contador temporal de posicion en strings
	CHAR oper;	//que operación va a hacer? $ o &

	//Afijo specificación que sigue el NFLAF.
	//Specifica como va a pegar el afijo o sufijo a la raíz.
	CHAR sAfxSpec[MAXAFFIX+1];

	//el afijo specificación si es un numero o
	//un contador de caracteres en el afijo spec. si es un string
	int nAfxSpec = 0;	


	while (CntSLine < LenLine) //va hasta el fin de línea
	{
     	//Saca el flag o \n (si es el fin de la frase).
		//Si el carácter es IFlg, hay un error porque
          //no permite: raiz /... /... 
		if ((oper=SLine[CntSLine]) == IspFlg && ErrSyntax)
		{
          	if (ErrMsg)
				fprintf(PMsgOutFile,
					"Error de sintaxis en línea %s de \"%s\":\n"
					"\tNo permite %c en este posición.\n",
					ltoa(LCntLines, SAffix,10), SInFile, IspFlg);
			return ERR_SYNTAX;
		}

		sAfxSpec[0] = NULL; //repone el afijo specificación al vacio.

		if (isFlag(oper))
		{
			//saca el afijo o afijo specificación
			for (cnt = 0, CntSLine++; CntSLine < LenLine &&
				!isFlag(SLine[CntSLine]); cnt++, CntSLine++)
			{                        	
				SAffix[cnt] = SLine[CntSLine];
			}

			SAffix[cnt] = NULL; //termina el string y quita el espacio
			stripSpace(SAffix);

			//Si hay an SpcFlg, mueve del afijo string al afijo
			//specificación string y sacar el afijo
			if (oper == SpcFlg)
			{				
				strcpy(sAfxSpec, SAffix);
				oper = SLine[CntSLine]; //saca el SfxFlg o IfxFlg

               	//Saca el afijo
				for (cnt = 0, CntSLine++; CntSLine < LenLine &&
					!isFlag(SLine[CntSLine]); cnt++, CntSLine++)
				{
					SAffix[cnt] = SLine[CntSLine];
				}

				SAffix[cnt] = NULL; //termina el string y quita el espacio
				stripSpace(SAffix);

				//error si no hay un afijo
                    //si no hay un sufijo o afijo flag o sufijo string es vacio
				if (((oper != SfxFlg && oper != IfxFlg && oper != RplFlg)
						|| SAffix[0] == NULL) && ErrSyntax)
				{
                    	if (ErrMsg)
						fprintf(PMsgOutFile,
							"Error de sintaxis en línea %s de \"%s\":\n"
							"\tNecesita un afijo después de \'%c\'.\n",
							ltoa(LCntLines, SAffix,10), SInFile, SpcFlg);
					return ERR_SYNTAX;
				}
			}

			if (SAffix[0] != NULL) //si no es un string vacio
			{
				strcpy(SNewWord, SRoot);

				if (oper == IfxFlg)//si insertando un infijo
				{
					if (sAfxSpec[0] != NULL) //si hay un afijo specificación
					{
						//Si el specificación es un numero, mueve atras
						//este numero de caracteres del fin y inserta el afijo.
						if (isdigit(sAfxSpec[0])) //no debe ser espacios acá
						{
							//Primero verifica que no hay errors en el numero.
							//Esto no sirve si hay un string como "0R4".
							if ((nAfxSpec = atoi(sAfxSpec)) == 0 &&
									sAfxSpec[0] != '0' && ErrSyntax)
							{
                                   	if (ErrMsg)
									fprintf(PMsgOutFile,
										"Error de sintaxis en línea %s de \"%s\":\n"
										"\t\"%s\" no es un número valido.\n",
										ltoa(LCntLines, SAffix, 10), SInFile, sAfxSpec);
								return ERR_SYNTAX;
							}
							//Verifica que el afijo specificación no es más grande
                                   //que el tamaño de la raíz
							if (nAfxSpec > strlen(SRoot) && ErrSyntax)
							{
                                   	if (ErrMsg)
									fprintf(PMsgOutFile,
										"Error de sintaxis en línea %s de \"%s\":\n"
										"\tNo puede insertar el infijo porque la "
										"raiz \"%s\" es menos que %d caracteres.\n",
										ltoa(LCntLines, SAffix, 10), SInFile,
										SRoot, nAfxSpec);
								return ERR_SYNTAX;
							}

							strIns(SNewWord, SAffix, strlen(SRoot) - nAfxSpec, -1, 0);
                              }
						else //afijo spec. es un string--buscalo y inserta antes
						{
                              	//si el afijo spec existe en la raíz
							if (strstr(SRoot,sAfxSpec) != NULL)
								//inserta el afijo
								strIns(SNewWord, SAffix, strStrLast(SRoot, sAfxSpec),
									-1, 0);
							else
								//si el afijo spec. no fue encontrado, inserta
                                        //el afijo antes del última carácter en la raiz
								strIns(SNewWord, SAffix, strlen(SRoot) - 1, -1, 0);
                              }
					}
					//si no hay un afijo specificación, inserta el afijo
					//antes del último carácter en la raíz.
					else
					{ 
						strIns(SNewWord, SAffix, strlen(SRoot) - 1, -1, 0);
                         }
				}
				//si la operacion es re-emplazar texto en la raiz al insertar el infijo
				else if (oper == RplFlg)
				{
					if (sAfxSpec[0] != NULL) //si hay un afijo specificación
					{
						//Si el specificación es un numero, mueve atras
						//este numero de caracteres del fin y inserta el afijo.
						if (isdigit(sAfxSpec[0])) //no debe ser espacios acá
						{
							//Primero verifica que no hay errors en el numero.
							//Esto no sirve si hay un string como "0R4".
							if ((nAfxSpec = atoi(sAfxSpec)) == 0 &&
									sAfxSpec[0] != '0' && ErrSyntax)
							{
                                   	if (ErrMsg)
									fprintf(PMsgOutFile,
										"Error de sintaxis en línea %s de \"%s\":\n"
										"\t\"%s\" no es un número valido.\n",
										ltoa(LCntLines, SAffix, 10), SInFile, sAfxSpec);
								return ERR_SYNTAX;
							}
							//Verifica que el afijo specificación no es más grande
                                   //que el tamaño de la raíz
							if (nAfxSpec > strlen(SRoot) && ErrSyntax)
							{
                                   	if (ErrMsg)
									fprintf(PMsgOutFile,
										"Error de sintaxis en línea %s de \"%s\":\n"
										"\tNo puede insertar el infijo porque la "
										"raiz \"%s\" es menos que %d caracteres.\n",
										ltoa(LCntLines, SAffix, 10), SInFile,
										SRoot, nAfxSpec);
								return ERR_SYNTAX;
							}

							strIns(SNewWord, SAffix, strlen(SRoot) - nAfxSpec,
								 -1, nAfxSpec);
                              }
						else //afijo spec. es un string--buscalo y quitalo de la raiz
						{
                              	//si el afijo spec existe en la raíz
							if (strstr(SRoot,sAfxSpec) != NULL)
								//inserta el afijo
								strIns(SNewWord, SAffix, strStrLast(SRoot, sAfxSpec),
									-1, strlen(sAfxSpec));
							else
								//si el afijo spec. no fue encontrado, inserta
                                        //el afijo antes del ultima carácter
								strIns(SNewWord, SAffix,strlen(SRoot) - 1, -1, 0);
                              }
					}
					//si no hay un afijo specificación, pega el afijo
					//antes del última carácter.
					else
					{ 
						strIns(SNewWord, SAffix, strlen(SRoot) - 1, -1, 0);
                         }
				 }
				 //si pegando un sufijo al fin de la raíz
				 else if (oper == SfxFlg)
				 {
					if (sAfxSpec[0] != NULL) //si hay un afijo specificación
					{
						//Si el specificación es un numero, borra 
						//este numero de caracteres del fin y pega el sufijo.
						if (isdigit(sAfxSpec[0])) //no debe ser espacios acá
						{
							//Primero verifica que no hay errors en el numero.
							//Esto no sirve si hay un string como "0R4".
							if (((nAfxSpec = atoi(sAfxSpec)) == 0 &&
									sAfxSpec[0] != '0') && ErrSyntax)
							{
                                   	if (ErrMsg)
									fprintf(PMsgOutFile,
										"Error de sintaxis en línea %s de \"%s\":\n"
										"\t\"%s\" no es un número valido.\n",
										ltoa(LCntLines, SAffix, 10),
										SInFile, sAfxSpec);
								return ERR_SYNTAX;
							}
							//Verifica que el afijo specificación no es más grande
                                   //que el tamaño de la raíz
							if (nAfxSpec > strlen(SRoot) && ErrSyntax)
							{
                                   	if (ErrMsg)
									fprintf(PMsgOutFile,
										"Error de sintaxis en línea %s de \"%s\":\n"
										"\t\No puede borrar %d characters de la "
										"raiz \"%s\".\n", ltoa(LCntLines, SAffix, 10),
										SInFile, nAfxSpec, SRoot);
								return ERR_SYNTAX;
							}

							strIns(SNewWord, SAffix, strlen(SRoot) - nAfxSpec,
								-1, nAfxSpec);
                              }
						else //afijo spec. es un string
							//buscalo y borrar hasta tal punto en la raíz
						{
							//Verifica que sfxSpec no es más grande que la raiz.
                                   //Puede causar problemas con strcmp().
							if (strlen(sAfxSpec) > strlen(SRoot))
								strIns(SNewWord, SAffix, -1, -1, 0);

							//si el afijo spec existe al fin de la raíz
							else if (strcmp(SRoot + sizeof(CHAR) * (strlen(SRoot) -
									strlen(sAfxSpec)), sAfxSpec) == 0)
							{
								//borra el afijo spec y pega el sufijo al fin
								strIns(SNewWord, SAffix, strStrLast(SRoot, sAfxSpec),
									-1, strlen(sAfxSpec));
                                   }
							else //el afijo spec. no existe al fin de la raíz, pega al fin 
								strIns(SNewWord, SAffix, -1, -1, 0);
                              }
					}
					//si no hay un afijo specificación, pega el sufijo despues
					//del último carácter en la raíz.
					else
					{ 
						strIns(SNewWord, SAffix,-1, -1, 0);
					}
				 }
				 else if(ErrSyntax) //No hay SfxFlg ni IfxFlg
				 {
                     	if (ErrMsg)
						fprintf(PMsgOutFile,"Error de sintaxis en línea %s de \"%s\":\n",
							ltoa(LCntLines, SAffix, 10), SInFile);
					return ERR_SYNTAX;
                     }

				//si error al escribir la nueva palabra, Ispell flags, y return
				if (fputs(SNewWord, POutFile) == EOF ||
					fputs(SIspellFlags,POutFile) == EOF ||
					fputc('\n',POutFile) == EOF)
				{
                    	if (ErrMsg)
						fprintf(PMsgOutFile,
							"Error escribiendo al archivo \"%s\"\n",SInFile);
					fclose(PInFile);
					fclose(POutFile);
					exit(ERR_WR_DIC);
				}
				LCntWords++;   //incrementar contadores
				LCntNewWords++;
				//printf("SRoot = \"%s\" flag = \"%s\" SAffix = \"%s\"\n",
				//	SRoot, SIspellFlags, SAffix);
			} //si SAffix es vacio, sigue al proximo afijo o al fin de línea
		}
		//si el proximo carácter no es un flag, no hace nada
	}
   	return RET_SUCCESS;
}

//Como ANSI strstr(), pero busca para sFind desde el fin de string sInStr.
//Vuelve el position en CHAR array donde el último fue encontrado.
//Si no sFind no fue encontrado en sInStr, vuelve -1. 
int strStrLast(CHAR * sInStr, CHAR * sFind)
{
	int cntInStr, cntFind;

	cntInStr = strlen(sInStr) - 1; //-1 porque arrays conta desde 0.
	
	while (cntInStr >= 0)
	{
     	//pasa atras hasta llega al ultimo character que coincide
		for (cntFind = strlen(sFind) - 1; cntInStr >= 0 && cntFind >= 0
			&& sInStr[cntInStr] != sFind[cntFind]; cntInStr--)
		{
			//printf("cntFind=%d, cntInStr=%d, sFind=%c, sInStr=%c,\n",
			//	cntFind, cntInStr, sFind[cntFind], sInStr[cntInStr]);
			; //no hace nada
		} 

		for (; cntInStr >= 0 && cntFind >= 0 &&
			(sInStr[cntInStr] == sFind[cntFind]); cntInStr--, cntFind--)
		{
			//printf("cntFind=%d, cntInStr=%d, sFind=%c, sInStr=%c,\n",
			//	cntFind, cntInStr, sFind[cntFind], sInStr[cntInStr]);
			if (cntFind == 0)
				return cntInStr;
		}
	}

	return -1;
}  

//saca los parámetros (argumentos) que fueron pasado a este programa
//por el usuario.
int getArgs(int argC, OS_CHAR **argV) 
{
	OS_CHAR sArgLine[MAXLINE];
	OS_CHAR sArg[MAXLINE];

	int cnt, 			//contador de arrays
     	cntInArg;		//contador de sArg

	//sArgLine = temp1;
	//sArg = temp2;    		
	//si quiere ayuda:	formdic -h, --help  //estilo UNIX
     //				formdic /?	    //estilo DOS
	if (argC >= 2 && !strcmp(argV[1],"-h") || !strcmp(argV[1],"--help") ||
		!strcmp(argV[1],"/?"))
	{
		helpMsg("Ayuda para formdic:");
		exit(RET_HELP);
	}

	//Si no hay suficient parametros
	if (argC <= 2)
	{
		helpMsg("Falta los nombres de archivos pre-diccionario y diccionario");
          exit(ERR_NO_DIC);
	}

	if ((SInFile = trim(argV[argC - 2])) == NULL ||
			strlen(SInFile) == 0 || SInFile[0] == '-')
	{
		helpMsg("Error: Falta el nombre del archivo pre-diccionario.");
          exit(ERR_NO_PREDIC);
	}

	if ((SOutFile = trim(argV[argC - 1])) == NULL ||
			strlen(SOutFile) == 0 || SOutFile[0] == '-')
	{
		helpMsg("Error: Falta el nombre del archivo diccionario.");
          exit(ERR_NO_DIC);
	}

	if (argC > 3)
	{
		sArgLine[0] = NULL;

		//cicla por todo los argumentos excepto el diccionario y prediccionario
		//y añadirlos a sArgLine
		for (cnt = 1; cnt < argC - 2 && strlen(sArgLine) < MAXLINE; cnt++)
		{
			printf("arg %d =\"%s\"\n", cnt, argV[cnt]);
			strcat(sArgLine, argV[cnt]);
          }

		stripSpace(sArgLine);

          printf("sArgLine=\"%s\"\n", sArgLine);

		for (cnt = 0; cnt < strlen(sArgLine);)
		{
			cntInArg = 0; //repone contador de sArg al cero
			sArg[0] = NULL; //repone sArg a vacio para un argumento nuevo

               //si no es un argumento valido                                                      
			if (sArgLine[cnt] != '-' || strlen(sArgLine) < cnt + 2)
			{
               	printf("cnt=%d,sArgLine[Cnt]=%c,\n",cnt,sArgLine[cnt]);
				fprintf(PMsgOutFile, "Parámetro(s) invalido(s): \"%s\"",
					sArgLine + (sizeof(OS_CHAR)*cnt));
				helpMsg("");
				exit(ERR_PARAMETER);
			} 

               //asignar '-' al sArg y incrementar
			sArg[cntInArg++] = sArgLine[cnt++];

               //si el proximo carácter es '-' tambien
			if (sArgLine[cnt] == '-')
				sArg[cntInArg++] = sArgLine[cnt++];

			sArg[cntInArg] = NULL; //termina el string

               //talvez va a implementar luego:
			//si el argumento puede contiene un nombre de archivo (--output=filename)
			//inFileName = tolower(sArgLine[cnt] == "o" ? 1 : 0;   

               //saca el argumento
			while (sArgLine[cnt] != '-' && sArgLine[cnt] != NULL)
				sArg[cntInArg++] = tolower(sArgLine[cnt++]);

			sArg[cntInArg] = NULL;


			if (!strncmp(sArg, "-f", 2))
			{
				checkParam(sArg, "-f=", 4, (sArg[3] < 0x21),
					"Error en sintaxis asignando el flag de Ispell: \"%s\"", sArg);
				IspFlg = sArg[3];
			}
			else if (!strncmp(sArg, "--flag", 6))
			{
				checkParam(sArg, "--flag=", strlen("--flag=") + 1,
					(sArg[strlen(sArg) - 1] < 0x21),
					"Error en sintaxis asignando el flag de Ispell: \"%s\"", sArg);
				IspFlg = sArg[strlen(sArg) - 1];
			}
			else if (!strncmp(sArg,"-a",2))
               {
				checkParam(sArg, "-a=", 4, (sArg[3] < 0x21),
					"Error en sintaxis asignando el flag de afijo specificación: \"%s\"", sArg);
				SpcFlg = sArg[3];
			}
			else if (!strncmp(sArg,"--affix",7))
               {
				checkParam(sArg, "--affix=", strlen("--affix=") + 1,
					(sArg[strlen(sArg) - 1] < 0x21),
					"Error en sintaxis asignando el flag de afijo specificación: \"%s\"", sArg);
				SpcFlg = sArg[strlen(sArg) - 1];;
			}
			else if (!strncmp(sArg,"-s",2))
               {
				checkParam(sArg, "-s=", 4, (sArg[3] < 0x21),
					"Error en sintaxis asignando el flag de sufijo: \"%s\"", sArg);
				SfxFlg = sArg[3];
			}
			else if (!strncmp(sArg,"--suffix",8))
               {
				checkParam(sArg, "--suffix=", strlen("--suffix=") + 1,
					(sArg[strlen(sArg) - 1] < 0x21),
					"Error en sintaxis asignando el flag de sufijo: \"%s\"", sArg);
				SfxFlg = sArg[strlen(sArg) - 1];;
			}
			else if (!strncmp(sArg,"-i",2))
			{
				checkParam(sArg, "-i=", 4, (sArg[3] < 0x21),
					"Error en sintaxis asignando el flag de infijo: \"%s\"", sArg);
				IfxFlg = sArg[strlen(sArg) - 1];
			}
			else if (!strncmp(sArg,"--infix",7)) 
			{
				checkParam(sArg, "--infix=", strlen("--infix=") + 1,
					(sArg[strlen(sArg) - 1] < 0x21),
					"Error en sintaxis asignando el flag de infijo: \"%s\"", sArg);
				IfxFlg = sArg[strlen(sArg) - 1];
			}
			else if (!strncmp(sArg,"-r",2))
			{
				checkParam(sArg, "-r=", 4, (sArg[3] < 0x21),
					"Error en sintaxis asignando el flag de re-emplazar: \"%s\"", sArg);
				RplFlg = sArg[3];
			}
			else if (!strncmp(sArg,"--replace",9))
			{
				checkParam(sArg, "--replace=", strlen("--replace=") + 1, (sArg[strlen(sArg) - 1] < 0x21),
					"Error en sintaxis asignando el flag de re-emplazar: \"%s\"", sArg);
				RplFlg = sArg[strlen(sArg) - 1];;
			}
			else if (!strncmp(sArg,"-c",2))
			{
				checkParam(sArg, "-c=", 4, (sArg[3] < 0x21),
					"Error en sintaxis asignando el flag de comentarios: \"%s\"", sArg);
				CmtFlg = sArg[3];
			}
			else if (!strncmp(sArg,"--comment",9))
			{
				checkParam(sArg, "--comment=", strlen("--comment=") + 1,
					(sArg[strlen(sArg) - 1] < 0x21),
					"Error en sintaxis asignando el flag de comentarios: \"%s\"", sArg);
				CmtFlg = sArg[strlen(sArg) - 1];
			}
			else if (!strncmp(sArg,"-m",2))
			{
				checkParam(sArg, "-m=", strlen("-m=yes"),
					strstr(sArg, "yes") == NULL && strstr(sArg, "no") == NULL,
					"Error de sintaxis en el parámetro de mensajes: \"%s\"", sArg);
				ErrMsg = strstr(sArg,"no") != NULL ? 0 : 1 ;
			}
			else if (!strncmp(sArg,"--messages",9))
			{
				checkParam(sArg, "--messages=", strlen("--messages=yes"),
					strstr(sArg, "yes") == NULL && strstr(sArg, "no") == NULL,
					"Error de sintaxis en el parámetro de mensajes: \"%s\"", sArg);
				ErrMsg = strstr(sArg,"no") != NULL ? 0 : 1 ;
			}
			else if (!strncmp(sArg,"-o",2))
			{
				MsgOutFile = MSG_TO_FILE;	//=1; opuesto es NO_MSG_TO_FILE

				if(strlen(sArg) > 2 && sArg[2] != '=')
				{
					fprintf(PMsgOutFile,
						"Error de sintaxis en el parámetro \"%s\"\n", sArg);
					helpMsg("");
					exit(ERR_PARAMETER);
                    }
				else if (sArg[2] == '=' && strlen(sArg) > 3)
				{
					strncpy(SMsgOutFile, sArg + sizeof(OS_CHAR) * 3, MAXFILENAME);
					if (strlen(sArg) > MAXFILENAME - 1) //si el string no es terminado
						SMsgOutFile[MAXFILENAME-1] = NULL;
				}
				else
					strcpy(SMsgOutFile,"fdic-msg.txt");
				

			}
			else if (!strncmp(sArg,"--output",strlen("--output")))
			{
				if (sArg[strlen("--output=") - 1] == '=' &&
					strlen(sArg) > strlen("--output="))
				{
					strncpy(SMsgOutFile, sArg + sizeof(OS_CHAR) * strlen("--output="),
						MAXFILENAME);
					if (strlen(sArg) > MAXFILENAME - 1) //si el string no es terminado
						SMsgOutFile[MAXFILENAME-1] = NULL;
				}
				else
					strcpy(SMsgOutFile,"fdic-msg.txt");
				MsgOutFile = 1; MSG_TO_FILE;	//=1; opuesto es NO_MSG_TO_FILE
			}
			else if (!strncmp(sArg,"-e",2))
			{
				checkParam(sArg, "-e=", strlen("-e=yes"),
					strstr(sArg, "yes") == NULL && strstr(sArg, "no") == NULL,
					"Error en sintaxis del parámetro de errores: \"%s\"", sArg);
				ErrSyntax = strstr(sArg,"no") != NULL ? 0 : 1;
			}
			else if (!strncmp(sArg,"--errors",8))
			{
				checkParam(sArg, "--errors=", strlen("--errors=yes"),
					strstr(sArg, "yes") == NULL && strstr(sArg, "no") == NULL,
					"Error en sintaxis del parámetro de errores: \"%s\"", sArg);
				ErrSyntax = strstr(sArg,"no") != NULL ? 0 : 1;
			}
			else 
			{  
				fprintf(PMsgOutFile,"Parámetro invalido: \"%s\"", sArg);
                    helpMsg("");
				exit(ERR_PARAMETER);
			}
		}//fin de FOR statement
     }//fin de IF statement

	return RET_SUCCESS;
}


//abre los archivos pre-diccionario para leer y diccionario para escribir
//y el archivo para recibir mensajes de pantalla. 
int openFiles (void)
{
	//si el usuario quiere redirigir los mensajes al archivo 
	if (MsgOutFile == MSG_TO_FILE)
	{
		if ((PMsgOutFile = fopen(SMsgOutFile, "wt")) == NULL)
		{
			if (ErrMsg == ERR_MSG_ON)
				fprintf(stderr,"Error: No puede abrir archivo \"%s\"\n",
					SMsgOutFile);
			exit(ERR_OPEN_MSG);
          }
	}
	
     //abrir en modo texto solo para leer
     //si hay error al abrir el pre-diccionario
	if ((PInFile = fopen(SInFile, "rt")) == NULL)
	{
		if (ErrMsg == ERR_MSG_ON)
			fprintf(PMsgOutFile,"Error: No puede abrir archivo \"%s\"\n",
				SInFile);
		exit(ERR_OPEN_PREDIC);
	}

	//Aviso: esto borra todo el archivo y re-escribir de nuevo.
	//si hay error al abrir el diccionario
	if ((POutFile = fopen(SOutFile, "wt")) == NULL)
	{
		if (ErrMsg == ERR_MSG_ON)
			fprintf(PMsgOutFile,"Error: No puede abrir archivo \"%s\"\n",
				SOutFile);
		exit(ERR_OPEN_DIC);
	}

	return RET_SUCCESS;
}


//verificar que sirve un parametro que fue pasado por el usuario
int checkParam(CHAR * sParam, CHAR * sCheck, int maxLen, int isBad,
	CHAR * errorMsg, CHAR * sBadArg)
{
	if (strncmp(sParam, sCheck,strlen(sCheck)) ||
			strlen(sParam) > maxLen || isBad)
	{
		fprintf(PMsgOutFile, errorMsg, sBadArg);
		helpMsg("");
		exit(ERR_PARAMETER);
	}

	return RET_SUCCESS;
}
				
    	



