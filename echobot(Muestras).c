#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <telebot.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/*//////////////////////////////////DEFINES///////////////////////////////////*/

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(array[0]))
#define FIFO_PATH "/tmp/MI_FIFO"

/*////////////////Definición de variables globales////////////////////////////*/

int flag1=3;																	//Variable bandera 1
int flag2=3;																	//Variable bandera 2
int RetCode=0;																	//Variable de retorno para control del programa
int cantidad_archivos;															//Entero donde se obtendrá la cantidad de archivos almacenables 
int lastfile;																	//Entero con el número del último archivo creado
int firstfile;																	//Entero con el número del archivo creado más antiguo
int number_file2;																//Entero que llevará el conteo de archivos
int lista;																		//Variable bandera para el listado de archivos
int nread;																		//Entero con la cantidad de caracteres leidos de la FIFO 
int fifo_t;																		//Descriptor de archivo para la apertura de la FIFO
int err;																		//Variable de retorno en la creación de la FIFO
int pid_m;																		//Entero con el PID del proceso de medición
char yourpid[20];																//Arreglo donde obtendremos el PID del proceso de medición 
char auxiliar_cantidad[50]={""};												//Arreglo auxiliar donde obtendremos la cantidad de archivos almacenables

telebot_handler_t handle;														//Este es un objeto opaco para representar a un manipulador de telebot.
telebot_user_t me;																//Este objeto representa un usuario o bot de Telegram.
telebot_error_e ret;															//Variable de retorno para control del programa. En caso de éxito, se devuelve #TELEBOT_ERROR_NONE; de lo contrario, un valor de error negativo.
telebot_message_t message;														//Este objeto representa un mensaje.
telebot_update_t *updates;														//Este objeto representa una actualización entrante.
telebot_update_type_e update_types[] = {TELEBOT_UPDATE_TYPE_MESSAGE};			//Representa el tipo de actualización.

///////////////////////////////FUNCIONES////////////////////////////////////////

/*////////////////////////Función "atoifunction"//////////////////////////////*/
int atoifunction(char *character)												//Función que permite implementar la función atoi dentro de un condicional
{
    return(atoi(character));													//Función atoi que permite obtener el valor numérico del arreglo de 
}																				//caracteres pasado como argumento

//////////////////////Función "obtener_cantidad"////////////////////////////////

void obtener_cantidad()														 	//Función que obtiene la cantidad de archivos almacenables
{
	FILE *flast;																//Descriptor del archivo Parametros.txt
	int i_parcial3=0;															//Entero auxiliar
	char auxiliar[150]={""};													//Arreglo auxiliar
	
	flast=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Parametros.txt","r+");	//Apertura del archivo Parametros.txt
	
	if(flast==NULL)																//Si flast=NULL, no pudo abrirse correctamente el archivo Parametros.txt
	{
		printf("No se pudo abrir archivo\n");
		RetCode=-1;																//Si no pudo abrirse el archivo RetCode toma el valor -1
	}
	do																			//Mientras la línea leída sea distinta de "Cantidad de archivos" 
	{																			//obtendrá línea por línea
		fgets(auxiliar,40,flast);
	} while(strstr(auxiliar,"Cantidad de archivos")==NULL);

	fclose(flast);																//Cerramos el descriptor de archivo
	
	while(auxiliar[23+i_parcial3]!='\0')										//A partir de la posición 23 hasta el final del arreglo 
	{																			//obtenemos la cantidad de archivos y lo guardamos en auxiliar_cantidad
		auxiliar_cantidad[i_parcial3]=auxiliar[23+i_parcial3];					
		i_parcial3++;
	}
	cantidad_archivos=atoi(auxiliar_cantidad);									//Obtenemos numéricamente la cantidad de archivos
}

/*////////////////////////Función "enviar_archivo"////////////////////////////*/

void enviar_archivo(char auxiliar[300])											//Función que envía el archivo cuyo nombre se pasa en su argumento
{
	int length=strlen(auxiliar);												//Longitud del arreglo auxiliar de entrada
	char auxiliar2[100]={""};													//2do arreglo auxiliar
	char auxiliar3[100]={""};													//3er arreglo auxiliar
	char doc[]={"/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras"};	//Arreglo con la ruta donde se encuentran los archivos a enviar
	char doc2[400];																//Arreglo auxiliar a enviar
	int i_parcial=0;															//Entero auxiliar
	
	strcpy(doc2,doc);
	while(auxiliar[length-i_parcial]!='/')										//Recorremos el arreglo auxiliar desde el final hasta encontra el carácter '/'
	{
		auxiliar2[i_parcial]=auxiliar[length-i_parcial-2];						//En auxiliar2 guardamos el nombre invertido del archivo a enviar 
		i_parcial++;
	}
	
	length=strlen(auxiliar2);													//Longitud del nombre del archivo
	
	for(i_parcial=0;i_parcial<length-1;i_parcial++)								//Invertimos el arreglo auxiliar2 y obtenemos el nombre correcto en auxiliar3
	{
		auxiliar3[i_parcial]=auxiliar2[length-i_parcial-2];
	}
	
	strcat(doc2,auxiliar3);														//Concatenamos el nombre del archivo con la ruta donde se encuentra
	ret=telebot_send_document(handle, message.chat->id, doc2, true, NULL, "","", false, 0,"");	//Enviamos el documento especificado en el tercer argumento
	
}

/*/////////////////////////Función "ultimo"///////////////////////////////////*/

void ultimo()																	//Función que envía el último archivo creado
{
	char auxiliar[150]={""};													//Arreglo auxiliar
	char auxiliar2[200]={""};													//2do arreglo auxiliar
	char cadena[]={"find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -wholename \"*00"};	//Arreglo con el comando y la ruta donde buscaremos el archivo
    FILE *fl;																	//Descriptor para el archivo lastfile.txt
	FILE *name_file;															//Descriptor para el archivo resultado.txt
	int number_file;															//Entero donde guardaremos el número del último archivo
	
	fl=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/lastfile.txt","r+");		//Apertura del archivo lastfile.txt
	if(fl==NULL)																//Si fl=NULL, no pudo abrirse el archivo lastfile.txt
	{
		printf("No se pudo abrir archivo\n");
		RetCode=-2;																//En caso de no poder abrir el archivo, RetCode toma el valor -2
	}
	
	fgets(auxiliar,40,fl);														//Obtención del número del archivo siguiente al último creado
	fclose(fl);																	//Cierre del descriptor de archivo
	number_file=atoi(auxiliar);													//Obtenemos numéricamente el número del archivo siguiente al último
	number_file--;																//Restamos en 1 para obtener el número del último archivo
	
	if(number_file<0)															//Si number_file es negativo, significa que el número obtenido de lastfile
	{																			//es igual a 0, por lo tanto el archivo anterior es el mayor en número
		obtener_cantidad();														//Obtención de la cantidad de archivos almacenables
		number_file=cantidad_archivos;											//number_file toma el valor de cantidad_archivos
	}
	
    name_file=fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt","r+");										//Apertura del archivo resultado.txt 
    if(name_file==NULL)															//Si name_file=NULL, no pudo abrirse el archivo resultado.txt
    {
        printf("No se pudo abrir archivo\n");
        RetCode=-3;																//En caso de no poder abrir el archivo, RetCode toma el valor -3
    }
    if(number_file==0)															//Si number_file=0 se envía el archivo '000'
    {
			system("find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -name \"*000-*\" >/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");	//Ejecutamos el comando find que buscará el archivo 
			fgets(auxiliar2,200,name_file);														//cuyo nombre empiece por '000' en la ruta especificada
    }																							//y guardará el nombre completo en el archivo resultado.txt
    else																		//En caso de que number_file sea distinto de cero:
    {
        sprintf(auxiliar_cantidad,"%d",number_file);							//Conversión de number_file a un arreglo de caracteres 		
		strcat(cadena,auxiliar_cantidad);										//Concatenación del comando find, ruta de búsqueda y número de archivo
		strcat(cadena,"-*\">/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");									//Concatenación de la cadena anterior con el nombre del archivo donde se guarda el resultado de la búsqueda
		printf("cadena es = %s\n",cadena);
		system(cadena);															//Aplicación del comando a través de la función system
		fseek(name_file,0,SEEK_SET);											//Reubicamos el puntero del archivo al inicio del mismo
        fgets(auxiliar2,200,name_file);											//Leemos la ruta y el nombre del último archivo
    }
    
	fclose(name_file);															//Cierre del archivo resultado.txt
	
	enviar_archivo(auxiliar2);													//Paso de la ruta y nombre del último archivo como argumento a la función 
}																				//que enviará el archivo

/*//////////////////////////Función "ultimos10"///////////////////////////////*/

void ultimos10()																//Función que envía los 10 archivos más nuevos
{
	FILE *fl;																	//Descriptor para el archivo lastfile.txt
	FILE *name_file;															//Descriptor para el archivo resultado.txt
	int i_parcial2=0;															//Entero auxiliar
	int number_file;															//Entero con el número del archivo a enviar
	char auxiliar[40]={""};														//Arreglo auxiliar
	char auxiliar2[200]={""};													//2do arreglo auxiliar
    char char_aux;																//Carácter auxiliar
	
	fl=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/lastfile.txt","r+");	//Apertura del archivo lastfile.txt en modo lectura/escritura
	
	if(fl==NULL)																//Si fl=NULL no pudo abrirse el archivo lastfile.txt
	{
		printf("No se pudo abrir archivo\n");
		RetCode=-4;																//En caso de no poder abrir el archivo, RetCode toma el valor -4
	}
	fgets(auxiliar,40,fl);														//Obtención del número del archivo siguiente al último creado
	fclose(fl);																	//Cierre del descriptor del archivo lastfile.txt
	
	number_file=atoi(auxiliar);													//Obtenemos numéricamente el número del archivo siguiente al último
	number_file--;																//Restamos en 1 para obtener el número del último archivo
	
	if(number_file<0)															//Si number_file es negativo, significa que el número obtenido de lastfile
	{																			//es igual a 0, por lo tanto el archivo anterior es el mayor en número
		obtener_cantidad();														//Obtención de la cantidad de archivos almacenables
		number_file=cantidad_archivos;											//number_file toma el valor de cantidad_archivos
	}
	
	for(i_parcial2=0;i_parcial2<10;i_parcial2++)								//Bucle for que busca y envía los 10 archivos
	{
		char cadena[]={"find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -wholename \"*00"};		//Areglo con el comando y la ruta donde buscaremos el archivo
        
        name_file=fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt","r+");									//Apertura del archivo resultado.txt
        if(name_file==NULL)														//Si name_file=NULL no pudo abrirse el archivo resultado.txt
        {
            printf("No se pudo abrir archivo\n");
            RetCode=-4;															//En caso de no poder abrir el archivo, RetCode toma el valor -4
        }
        
		if(number_file==0)														//Si number_file=0 debemos continuar enviando desde el mayor archivo 
		{																		//numérico, en caso de que exista 
			system("find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -name \"*000-*\" >/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");	//Búsqueda del archivo '000' y su nombre y ruta se guardan
																				//en el archivo resultado.txt
			fgets(auxiliar2,200,name_file);										//Lectura del nombre y ruta del archivo '000'
			enviar_archivo(auxiliar2);											//Dicho nombre y ruta es pasado como argumento a la función que enviará el archivo
			
			obtener_cantidad();													//Obtención de la cantidad máxima de archivos almacenables
            sprintf(auxiliar_cantidad,"%d",cantidad_archivos);					//Conversión de la cantidad máxima de archivos a un arreglo de caracteres
            strcat(cadena,auxiliar_cantidad);									//Concatenación del comando find, ruta de búsqueda y número de archivo
			strcat(cadena,"-*\">/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");								//Concatenación de la cadena resultante anterior con el nombre del archivo donde se guardará el resultado
            system(cadena);														//Aplicación del comando
            
            fseek(name_file,0,SEEK_SET);										//Reubicación del puntero del archivo al inicio del mismo
            char_aux=fgetc(name_file);											//Obtención del primer carácter 
	
            if(char_aux=='/')													//Si el primer carácter es '/' significa que existe un archivo con el nombre buscado
            {																	 
				number_file=cantidad_archivos;									//number_file toma el valor del mayor archivo numérico
                i_parcial2++;
            }
			else
			{
				break;															//Si el primer carácter es distinto de '/', no se encontró ningún archivo 
			}																	//con el nombre buscado, por lo tanto estamos en el caso de que existen menos
		}																		//de 10 archivos almacenados y ya se enviaron todos
		
		sprintf(auxiliar_cantidad,"%d",number_file);							//Conversión de number_file a un arreglo de carateres
		strcat(cadena,auxiliar_cantidad);										//Concatenación del comando find, ruta de búsqueda y número de archivo
		strcat(cadena,"-*\">/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");									//Concatenación de la cadena resultante anterior con el nombre del archivo donde se guardará el resultado
		system(cadena);															//Aplicación del comando
		
		fseek(name_file,0,SEEK_SET);											//Reubicación del puntero del archivo al inicio del mismo
        fgets(auxiliar2,200,name_file);											//Lectura del nombre y ruta del archivo 
        enviar_archivo(auxiliar2);												//Dicho nombre y ruta es pasado como argumento a la función que enviará el archivo
        number_file--;															//Disminución en 1 del número de archivo a enviar
        fclose(name_file);														//Cierre del descriptor de arhivo
	}
}

/*////////////////////////Función "listar_archivos"///////////////////////////*/

void listar_archivos()															//Función que muestra el listado de archivos y permite mostrar los 
{																				//10 archivos siguientes o los 10 anteriores
	FILE *fu;																	//Descriptor para el archivo lastfile.txt
	FILE *name_file2;															//Descriptor para el archivo resultado.txt
	int i_parcial3=0;															//Entero auxiliar
	int i_parcial4=0;															//2do entero auxiliar
	int i_parcial5=0;															//3er entero auxiliar
	int cant_aux;																//Entero auxiliar para cantidad
	int length;																	//Entero auxiliar para longitud de arreglos
	char auxiliar[40]={""};														//Arreglo auxiliar
	char auxiliar2[200]={""};													//2do arreglo auxiliar
	char char_aux;																//Carácter auxiliar
	char char_aux2;																//2do carácter auxiliar
	char oldest[]={"find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -wholename \"*00"};	//Arreglo con el comando y la ruta donde buscaremos el archivo
	
	if(lista==-1)																//Si lista=-1, estamos en el primer caso donde hemos llamado a la función listar_archivos
	{
		fu=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/lastfile.txt","r+");	//Apertura del archivo lastfile.txt
	
		if(fu==NULL)															//Si fu=NULL, no pudo abrirse el archivo lastfile.txt
		{
			printf("No se pudo abrir archivo\n");
			RetCode=-5;															//Si no pudo abrirse el archivo lastfile.txt, RetCode toma el valor -5
		}
		fgets(auxiliar,40,fu);													//Lectura del número del archivo siguiente al último
		fclose(fu);																//Cierre del archivo lastfile.txt
		number_file2=atoi(auxiliar);											//Obtención numérica del siguiente archivo al último creado
		
        strcat(oldest,auxiliar);												//Concatenación del comando find, la ruta de búsqueda y el número del archivo
        strcat(oldest,"-*\">/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");									//Concatenación del arreglo resultante anterior y el nombre del archivo resultado.txt
        system(oldest);															//Aplicación del comando
       
		name_file2=fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt","r+");									//Apertura del archivo resultado.txt
        char_aux2=fgetc(name_file2);											//Obtención del primer carácter del archivo resultado.txt
        fclose(name_file2);														//Cierre del archivo resultado.txt
        
        if(char_aux2=='/')														//Si el primer carácter es '/', existe el archivo que le sigue 
		{																		//numéricamente al último generado
			firstfile=number_file2;												//El entero firstfile toma el valor numérico del archivo más antiguo
		}
		else
		{
			firstfile=0;														//Si el archivo siguiente al más nuevo no existe, el más antiguo es el 0
		}
		
        number_file2--;															//Disminución en 1 de number_file2 para obtener el número del
																				//últino archivo creado
		obtener_cantidad();														//Llamada a la función que obtiene la cantidad de archivos almacenables
		if(number_file2<0)														//Si number_file2 es negativo, significa que el archivo más nuevo es el 0
		{
			number_file2=cantidad_archivos;										//Por lo anterior, number_file2 toma el valor del mayor archivo numérico
		}
		lastfile=number_file2;													//lastfile toma el valor numérico del archivo más nuevo
	}
	
	char auxiliar4[1000]={""};													//4to arreglo auxiliar
	
	for(i_parcial3=0;i_parcial3<10;i_parcial3++)								//Bucle for que enviará los nombres de los 10 archivos
	{
		char cadena[]={"find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -wholename \"*00"};	//Arreglo con el comando y la ruta donde buscaremos el archivo
		char aux_name[400]={""};												//Arreglo auxiliar para el nombre del archivo
		char aux_name2[400]={""};												//2do arreglo auxiliar para el nombre del archivo
		char auxiliar3[50]={""};												//3er arreglo auxiliar
		
		name_file2=fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt","r+");									//Apertura del archivo resultado.txt
		if(name_file2==NULL)													//Si name_file2=NULL, no pudo abrirse el archivo resultado.txt
		{
			printf("No se pudo abrir archivo\n");
			RetCode=-6;															//Si no pudo abrirse el archivo resultado.txt, RetCode toma el valor -6
		}
        
        if(number_file2==0)														//Si number_file2=0 se debe buscar y enviar el nombre del archivo '000'
		{
			flag2=0;															//Bajada de la bandera número 2
			system("find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -name \"*000-*\" >/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");	//Búsqueda del archivo '000'
			
			fgets(auxiliar2,150,name_file2);									//Lectura del nombre del archivo del archivo resultado.txt
			length=strlen(auxiliar2);											//Obtención de la longitud del nombre y la ruta del archivo '000'
			
			i_parcial4=0;
			while(auxiliar2[length-i_parcial4]!='/')							//Se recorre el arreglo auxiliar desde el final hasta encontrar el carácter '/'
			{																	//y se guarda el nombre del archivo en el arreglo auxiliar3
				auxiliar3[i_parcial4]=auxiliar2[length-i_parcial4-2];			
				i_parcial4++;
			}
           
			length=strlen(auxiliar3);											//Obtención de la longitud del nombre del archivo '000'
			for(i_parcial4=0;i_parcial4<length-1;i_parcial4++)					//Puesto que el arreglo auxiliar3 posee el nombre invertido del archivo, 
			{																	//debemos volver a invertirlo
				aux_name2[i_parcial4]=auxiliar3[length-i_parcial4-2];
			}
           
		strcat(aux_name2,"\n\n");											//Concatenación de dos saltos de linea
			strcat(aux_name2,auxiliar4);										//Concatenación del nombre del archivo con los nombres de los archivos anteriores
			strcpy(auxiliar4,aux_name2);										//Copia del arreglo resultante anterior en el arreglo auxiliar4
            
			obtener_cantidad();													//Obtención de la cantidad de archivos almacenables
			cant_aux=cantidad_archivos;											//Asignación de la cantidad máxima a cant_aux
			sprintf(auxiliar_cantidad,"%d",cantidad_archivos);					//Conversión de la cantidad máxima de archivos a un arreglo de caracteres
			strcat(cadena,auxiliar_cantidad);									//Concatenación del comando find, la ruta de búsqueda y el número del archivo
			strcat(cadena,"-*\">/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");								//Concatenación del arreglo resultante anterior con el nombre del archivo resultado.txt
			system(cadena);														//Aplicación del comando
			
			fseek(name_file2,0,SEEK_SET);										//Reubicación del puntero del archivo al inicio de este
			
			char_aux=fgetc(name_file2);											//Obtención del primer carácter del archivo
			if(char_aux=='/')													//Si el primer carácter es '/' significa que existe un archivo con el nombre buscado
			{
				number_file2=cantidad_archivos;									//number_file2 toma el valor numérico del mayor archivo numérico
			}
			else																//Si no existe el archivo buscado, significa que el último archivo a enviar es el 0 
			{																	//y ya se enviaron todos los archivos
				if(i_parcial3!=9)												//Si el archivo número 0 no es el último archivo a enviar
				{
					for(i_parcial5=0;i_parcial5<10;i_parcial5++)										//Bucle for que recorre desde el máximo número de archivo hasta la cantidad máxima menos 10
					{	
						char cadena2[]={"find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -wholename \"*00"};	//Cadena auxiliar 2
						char aux_name3[400]={""};														//Arreglo auxiliar para el nombre del archivo número 3
						char auxiliar5[50]={""};														//Arreglo auxiliar 5
						
						cant_aux--;																		//Disminución en 1 de la cantidad auxiliar
						
						fclose(name_file2);																//Cierre del archivo resultado.txt para eliminar contenido anterior
						name_file2=fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt","r+");											//Apertura del archivo resultado.txt
						sprintf(auxiliar_cantidad,"%d",cant_aux);										//Conversión de cant_aux en arreglo de caracteres
						strcat(cadena2,auxiliar_cantidad);												//Concatenación del comando, la ruta de búsqueda y el número de archivo
						strcat(cadena2,"-*\">/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");											//Concatenación del arreglo resultante anterior con el nombre del archivo resultado.txt
						system(cadena2);																//Aplicación del comando
						fseek(name_file2,0,SEEK_SET);													//Reubicación del puntero del archivo al inicio del mismo
						char_aux=fgetc(name_file2);														//Obtención del primer carácter
						
						if(char_aux=='/')																//Si el primer carácter es '/', existe el archivo buscado
						{
							fgets(auxiliar2,150,name_file2);											//Obtención de la ruta y el nombre del archivo
							length=strlen(auxiliar2);													//Longitud de la ruta y el nombre del archivo
							
							i_parcial4=0;
							while(auxiliar2[length-i_parcial4]!='/')										//Obtención del nombre del archivo por separado e invertido
							{
								auxiliar5[i_parcial4]=auxiliar2[length-i_parcial4-2];
								i_parcial4++;
							}
							
							length=strlen(auxiliar5);
							for(i_parcial4=0;i_parcial4<length-1;i_parcial4++)							//Obtención del nombre del archivo correcto
							{
								aux_name3[i_parcial4]=auxiliar5[length-i_parcial4-2];
							}
							
							strcat(aux_name3,"\n\n");													//Concatenación del nombre con dos saltos de línea
							strcat(aux_name3,auxiliar4);												//Concatenación de los nombres obtenidos anteriormente con el nuevo nombre
							strcpy(auxiliar4,aux_name3);
							number_file2=cant_aux;												//Se copia el arreglo resultante anterior en el arreglo auxiliar4
						}
					}
					break;																				//Salida del bucle 
				}													
			}
		}
		else																	//Si number_file2 es distinto de 0 debe buscarse el nombre del siguiente archivo
		{																		//a enviar y armar el arreglo con los nombres de los 10 archivos buscados
			sprintf(auxiliar_cantidad,"%d",number_file2);						//Conversión de number_file2 a un arreglo de caracteres 
			strcat(cadena,auxiliar_cantidad);									//Concatenación del comando find, la ruta de búsqueda y el número del archivo
			strcat(cadena,"-*\">/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");								//Concatenación del arreglo resultante anterior con el nombre del archivo resultado.txt
			system(cadena);														//Aplicación del comando
			
			fseek(name_file2,0,SEEK_SET);										//Reubicación del puntero del archivo al inicio de este
			fgets(aux_name,150,name_file2);										//Lectura del nombre del archivo del archivo resultado.txt
		
			i_parcial4=0;
			length=strlen(aux_name);											//Obtención de la longitud del nombre y la ruta del archivo 
			
            while(aux_name[length-i_parcial4]!='s')								//Se recorre el arreglo auxiliar desde el final 
            {																	//hasta encontrar el carácter 's' del directorio "Muestras"
                auxiliar3[i_parcial4]=aux_name[length-i_parcial4-1];
                i_parcial4++;
            }
        
			length=strlen(auxiliar3);											//Obtención de la longitud del nombre del archivo
			
			i_parcial4=0;
            while(auxiliar3[i_parcial4]!='/')									//Inversión del arreglo auxiliar3 para obtener el nombre correcto del archivo
            {
                aux_name2[i_parcial4]=auxiliar3[length-i_parcial4-2];
                i_parcial4++;
            }
            
			strcat(aux_name2,"\n\n");											//Concatenación de dos saltos de linea
			strcat(aux_name2,auxiliar4);										//Concatenación del arreglo auxiliar4 con todos los nombres ya buscados al nuevo nombre 
			strcpy(auxiliar4,aux_name2);										//Se copia el arreglo resultante anterior en el arreglo auxiliar4
			number_file2--;														//Disminución en 1 del numero de archivo
		}
		fclose(name_file2);														//Cierre del archivo resultado.txt
	}
	printf("number file vale %d\n",number_file2);
	ret = telebot_send_message(handle, message.chat->id, auxiliar4, "HTML", false, false, 0, "");	//Envío del arreglo con la lista de nombres de los 10 archivos buscados
}

/*//////////////////////////Función "mostrar_actuales"////////////////////////*/

void mostrar_actuales()															//Función que muestra los parámetros de configuración actuales
{
	char parameters_read[1000]={""};											//Arreglo donde se almacenarán los parámetros a mostrar
	FILE *fparameters;															//Descriptor para el archivo Parametros.txt
	int nro_caracteres;															//Cantidad de caracteres leidos del archivo Parametros.txt
	
	fparameters=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Parametros.txt","r+");	//Apertura del archivo Parametros.txt
	if(fparameters==NULL)														//Si fparameters=NULL, no pudo abrirse el archivo Parametros.txt
	{
		printf("\nNo se pudo abrir el archivo Parametros.txt\n");
		RetCode=-7;																//Si no pudo abrirse el archivo Parametros.txt, RetCode toma el valor -7
	}
	
	nro_caracteres=fread(parameters_read,sizeof(char),1000,fparameters);		//Lectura de los parámetros y almacenamiento en el arreglo parameters_read
	
	if(nro_caracteres==0)														
	{
		RetCode=-8;																//Si la cantidad de caracteres leídos es 0, RetCode toma el valor -8
	}
	fclose(fparameters);														//Cierre del archivo Parametros.txt
	
	ret = telebot_send_message(handle, message.chat->id, parameters_read, "HTML", false, false, 0, "");	//Envío de los parámetros leídos
	
}

/*////////////////////////Función "modificar_parametros"//////////////////////*/

void modificar_parametros()														//Función que presenta al usuario el listado de los parámetros a modificar
{
    ret = telebot_send_message(handle, message.chat->id, "Elija el parametro a modificar:\n /Canal_de_disparo\n /Frecuencia_de_muestreo\n /Nivel_de_disparo\n /Muestras_post_trigger\n /Muestras_pre_trigger\n /Cantidad_archivos_almacenables\n" , "HTML", false, false, 0, "");
}

/*//////////////////////Función "modify_parameters"///////////////////////////*/

void modify_parameters(int flag_change)											//Función que modifica los parámetros en el archivo. Recibe como argumento la 
{																				//bandera flag_change que representa al parámetro a modificar
	char auxiliar[300];															//Arreglo auxiliar
	FILE *fparameters;															//Descriptor para el archivo Parametros.txt
	char line[1000]={""};														//Línea auxiliar general
	char line1[200]={"1-Canal-"};												//1ra línea del archivo
	char line2[200]={"2-Frecuencia-"};											//2da línea del archivo
	char line3[200]={"3-Nivel-"};												//3ra línea del archivo
	char line4[200]={"4-Muestras posttrigger-"};								//4ta línea del archivo
	char line5[200]={"5-Muestras pretrigger-"};									//5ta línea del archivo	char line6[200]={"6-Cantidad de archivos-"};								//6ta línea del archivo
	int index=0; 																//Entero que nos permite, dentro del objeto que compone la actualización, elegir el mensaje
    int count;																	//Número de actualizaciones recibidas con #telebot_get_updates.
	int offset = -1;															//Identificador de la primera actualización que se devolverá. El desplazamiento negativo se puede especificar para recuperar actualizaciones a partir de la actualización de desplazamiento desde el final de la cola de actualizaciones.
	int i_parcial;																//Entero auxiliar
	int length;																	//Entero auxiliar con la longitud de un arreglo
	int length2;																//2do entero auxiliar con la longitud del arreglo
	int unusable;																//Entero utilizado para la determinación del tipo de parámetro
    char char_aux;																//Carácter auxiliar
    
	message = updates[index].message;											//Nuevo mensaje entrante de cualquier tipo: texto, foto, sticker, etc. 
	strcpy(message.text,line);													//Vaciamos el texto del mensaje, para evitar cualquier posible carácter residual 
    
	fifo_t = open(FIFO_PATH,O_RDWR | O_NONBLOCK, 0); 							//Apertura de la FIFO en modo lectura/escritura y no bloqueante
	if(fifo_t == -1)															//Si fifo_t=-1, ocurrió un error al intentar abrir la FIFO
	{
		printf("\nError al abrir FIFO\n");
		RetCode=-9;																//Si no pudo abrirse la FIFO, RetCode toma el valor -9
	}
	
	nread = read(fifo_t, yourpid, sizeof(yourpid));								//Lectura de la FIFO
	if(nread == -1) 															//Si nread=-1, ocurrió un error al intentar leer de la FIFO
	{
		printf("\nError al leer de la FIFO\n");
		RetCode=-10;															//Si no se pudo leer de la FIFO, RetCode toma el valor -10
	}
	else
	{
		pid_m=atoi(yourpid);													//Conversión numérica del PID del programa de medición
	}														
    
	while(1)
	{
        ret=telebot_get_updates(handle, offset, 20, 0, update_types, 0, &updates, &count);	//Obtención de actualizaciones (mensajes entrantes)
		
        message = updates[index].message;										//Asignación a message del nuevo mensaje de texto entrante
        
		if(flag_change==1||flag_change==2)										//Si se desea modificar el parámetro de canal de disparo o frecuencia de muestreo
		{
            char_aux=message.text[1];											//Asignación de la posición 1 del mensaje entrante al carácter auxiliar
            unusable=atoifunction(&char_aux);									//Conversión numérica del 2do carácter recibido
			if((unusable>=1&&unusable<=9)||char_aux=='0')						//Si el carácter recibido es un número, se sale del bucle
			{
				break;
			}
		}
		else																	//Si se desea modificar alguno de los otros parámetros
		{
			char_aux=message.text[0];											//Asignación de la posición 0 del mensaje entrante al carácter auxiliar
            unusable=atoifunction(&char_aux);									//Conversión numérica del 1er carácter recibido
			if((unusable>=1&&unusable<=9)||char_aux=='0')						//Si el carácter recibido es un número, se sale del bucle
			{
				break;
			}
		}
            
	}
	
	fparameters=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Parametros.txt","r+");	//Apertura del archivo Parametros.txt
	if(fparameters==NULL)														//Si fparameters=NULL, no pudo abrirse el archivo Parametros.txt
	{
		printf("\nNo se pudo abrir el archivo Parametros.txt\n");
		RetCode=-11;															//Si no pudo abrirse el archivo Parametros.txt, RetCode toma el valor -11
	}
	
	switch(flag_change)															//En base a la bandera flag_change que representa a cada parámetro:
	{
		case 1:																	//Parámetro del canal de disparo
		{	
			strcpy(line,line1);													//Copia de la linea 1 en la línea a copiar en el archivo
			length=strlen(message.text);										//Longitud del parámetro recibido
			for(i_parcial=0;i_parcial<length;i_parcial++)						//Corrimiento de un lugar hacia la izquierda de los caracteres recibidos
			{																	//para eliminar el carácter '/'
				message.text[i_parcial]=message.text[i_parcial+1];
			}	
		}	
		break;
		case 2:																	//Parámetro de la frecuencia de muestreo
		{	
			strcpy(line,line2);													//Copia de la linea 2 en la línea a copiar
			length=strlen(message.text);										//Longitud del parámetro recibido
			for(i_parcial=0;i_parcial<length;i_parcial++)						//Corrimiento de un lugar hacia la izquierda de los caracteres recibidos
			{																	//para eliminar el carácter '/'
				message.text[i_parcial]=message.text[i_parcial+1];
			}	
		}
		break;
		case 3:																	//Parámetro del nivel de disparo
		{
			strcpy(line,line3);													//Copia de la linea 3 en la línea a copiar en el archivo
		}
		break;
		case 4:																	//Parámetro de la cantidad de muestras post disparo
		{
			strcpy(line,line4);													//Copia de la linea 4 en la línea a copiar en el archivo
		}	
		break;				
		case 5:																	//Parámetro de la cantidad de muestras pre disparo
		{
			strcpy(line,line5);													//Copia de la linea 5 en la línea a copiar en el archivo
		}	
		break;
		case 6:																	//Parámetro de la cantidad de archivos almacenables
		{
			strcpy(line,line6);													//Copia de la linea 6 en la línea a copiar en el archivo
		}	
		break;
	}
	
	strcat(line,message.text);													//Concatenación del parámetro recibido con la línea a copiar en el archivo
    strcat(line,"\n");															//Concatenación del salto de línea con la línea a copiar en el archivo
	
	for(i_parcial=0;i_parcial<flag_change;i_parcial++)							//Recorrido del archivo Parámetros.txt hasta llegar a la línea a modificar
	{
		fgets(auxiliar,250,fparameters);
	}

	length2=strlen(auxiliar);													//Longitud de la línea actual a modificar
	
    for(i_parcial=0;i_parcial<(14-flag_change);i_parcial++)						//Recorrido del archivo Parámetros.txt desde el final de la línea a modificar
    {																			//hasta el final del mismo
        fgets(auxiliar,300,fparameters);
        length2=length2+strlen(auxiliar);										//Cálculo de la cantidad de caracteres recorridos
        strcat(line,auxiliar);													//Concatenación de cada línea posterior a la modificada
    }

    fseek(fparameters,-length2,SEEK_CUR);										//Reubicación del puntero del archivo hasta el principio de la línea modificada
    fputs(line,fparameters);													//Escritura de los parámetros concatenados con la línea modificada en el archivo
	
	fclose(fparameters);														//Cierre del archivo Parametros.txt
	
	ret = telebot_send_message(handle, message.chat->id, "Aplicar los cambios:\n\n/Aplicar_ahora.\n\n/Tal_vez_luego." , "HTML", false, false, 0, "");	//Envío de las opciones posteriores al usuario 
}

/*/////////////////////////////Funcion main///////////////////////////////////*/

int main(int argc, char *argv[])
{
////////////////////Declaración de variables locales////////////////////////////
	FILE *file_main;															//Descriptor para el archivo resultado.txt
	FILE *fp;																	//Descriptor para el archivo token.txt
	FILE *fm;																	//Descriptor para el archivo menu.txt
	FILE *ft;																	//Descriptor para el archivo temperatura.txt
	char name_file[400]={""};													//Arreglo auxiliar para el nombre del archivo a enviar
	char comando[]={"vcgencmd measure_temp > /home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/temperatura.txt"};					//Arreglo con el comando que arroja la temperatura de la placa
	char token[1024];															//Arreglo auxiliar para la obtención del token del bot
	char auxiliar[30];															//Arreglo auxiliar
	char auxiliar2[200]={""};;														//2do arreglo auxiliar
	char str[4096];																//Arreglo auxiliar con mensaje de bienvenida
	int flag_parameters=0;														//Variable bandera 
	int flag_change;															//Variable bandera 
	int index;																	//Entero que nos permite, dentro del objeto que compone la actualización, elegir el mensaje
	int count;																	//Número de actualizaciones recibidas con #telebot_get_updates.
	int offset = -1;															//Identificador de la primera actualización que se devolverá. El desplazamiento negativo se puede especificar para recuperar actualizaciones a partir de la actualización de desplazamiento desde el final de la cola de actualizaciones.
    
/////////////////////Creación e inicio del bot de Telegram//////////////////////
	
    printf("Welcome to Raspybot\n");
  
	err = mkfifo(FIFO_PATH,0777);												//Creación de la FIFO. En el 2do argumento se especifican los permisos de lectura, escritura y ejecución
	if(err == -1) 																//Si err=-1, no pudo crearse la FIFO
	{
		printf("\nError al crear FIFO, la FIFO ya existe\n");
	}

    fp = fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/token.txt", "r");												//Apertura del archivo token.txt, el cual contiene el token del bot
    if (fp == NULL)																//Si fp=NULL, no pudo abrirse el archivo token.txt
    {
        printf("Failed to open .token file\n");
		RetCode=-12;															//Si no pudo abrirse el archivo token.txt, RetCode toma el valor -12
		return RetCode;															//Se retorna RetCode y termina el programa ya que la obtención del token 
    }																			//es completamente necesaria para el funcionamiento del bot
    
    if (fscanf(fp, "%s", token) == 0)											//Lectura del token.
    {
        printf("Failed to read token\n");
        fclose(fp);																//Cierre del archivo token.txt
        RetCode=-13;															//Si no pudo leerse el archivo token.txt, RetCode toma el valor -13
		return RetCode;															//Se retorna RetCode y termina el programa ya que la obtención del token
    }																			//es completamente necesaria para el funcionamiento del bot
    printf("Token: %s\n", token);
    fclose(fp);																	//Cierre del archivo token.txt

    if (telebot_create(&handle, token) != TELEBOT_ERROR_NONE)					//Creación del bot de Telegram. Se le pasa como argumento un manejador y el token obtenido
    {
        printf("Telebot create failed\n");
		RetCode=-14;															//Si no pudo crearse el bot, RetCode toma el valor -14
		return RetCode;															//Se retorna RetCode y termina el programa ya que la creación del bot
    }																			//es completamente necesaria para el funcionamiento del programa
    if (telebot_get_me(handle, &me) != TELEBOT_ERROR_NONE)						//Obtención de la información del bot. Se le pasa como argumento el manejador 
    {																			//y el objeto me, donde se obtendrán los datos del bot
        printf("Failed to get bot information\n");
        telebot_destroy(handle);
		RetCode=-15;															//Si no pudo obtenerse información del bot, RetCode toma el valor -15
		return RetCode;															//Se retorna RetCode y termina el programa ya que la no obtención de 
	}																			//información del bot es un indicador de una creación defectuosa del bot

    printf("ID: %d\n", me.id);
    printf("First Name: %s\n", me.first_name);
    printf("User Name: %s\n", me.username);

    telebot_put_me(&me);														//Esta función se utiliza para liberar la memoria utilizada para la 
																				//información obtenida sobre el propio bot de telegram.
////////////////////////////////Bucle principal/////////////////////////////////   
	while (1)
    {
        char cadena_main[]={"find /home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/ -type f -wholename \"*"};		//Arreglo con el comando y la ruta donde buscaremos el archivo
        ret = telebot_get_updates(handle, offset, 20, 0, update_types, 0, &updates, &count);//Obtención de actualizaciones (mensajes entrantes)	
        if (ret != TELEBOT_ERROR_NONE)														//Si ret es distinto de TELEBOT_ERROR_NONE se saltan todas las 
            continue;																		//instrucciones dentro del bucle hasta la siguiente iteración
        printf("Number of updates: %d\n", count);
        
		for (index = 0; index < count; index++)												//Se obtienen los mensajes entrantes desde 0 hasta la cantidad 
        {																					//de actualizaciones recibidas
            message = updates[index].message;												//Dentro del objeto de la actualización entrante, obtenemos el mensaje recibido
            
            if (message.text)																//Si dentro de el mensaje recibido se tiene texto:
            {
                printf("%s: %s \n", message.from->first_name, message.text);
                
				if (strstr(message.text, "/start"))											//Si se recibe el comando /start, se envía un mensaje de bienvenida
				{
					snprintf(str, SIZE_OF_ARRAY(str), "Bienvenido a Raspybot %s", message.from->first_name);
					ret = telebot_send_message(handle, message.chat->id, str, "HTML", false, false, updates[index].message.message_id, "");
					if (ret != TELEBOT_ERROR_NONE)
					{
						printf("Failed to send message: %d \n", ret);
					}
				}
/////////////////////////////////Menú principal/////////////////////////////////				
				if (strstr(message.text, "Menu"))											//Si se recibe el mensaje "Menu", se presenta el menú principal al usuario
				{
					fm = fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/menu.txt", "r");											//Apertura del archivo menu.txt con el menú de opciones
					fread(auxiliar2,sizeof(char),200,fm);									//Lectura de las opciones
					printf("auxiliar 2 = %s\n",auxiliar2);
					ret = telebot_send_message(handle, message.chat->id, auxiliar2, "HTML", false, false, 0, "");	//Envío de las opciones al usuario
					fclose(fm);																//Cierre del archivo menu.txt
					flag1=3;																//Se reinicia la bandera 1
				}   
				
/////////////////////////////Descarga de archivos///////////////////////////////
				
					if (strstr(message.text, "/Bajar_archivos"))							//Si se recibe el comando /Bajar_archivos, la bandera 1 se coloca en 0
					{
						flag1=0;
					}
					
						if (strstr(message.text, "/Bajar_el_ultimo"))						//Si se recibe el comando /Bajar_el_ultimo, se reinicia la bandera 1 
						{																	//y se llama a la función que envía el último archivo
							ret = telebot_send_message(handle, message.chat->id, "Ultimo archivo" , "HTML", false, false, 0, "");
							flag1=3;
							ultimo();
						}
						
						if (strstr(message.text, "/Bajar_ultimos_10"))						//Si se recibe el comando /Bajar_ultimos_10, se reinicia la bandera 1
						{																	//y se llama a la función que envía los últimos 10 archivos
							ret = telebot_send_message(handle, message.chat->id, "Ultimos 10 archivos" , "HTML", false, false, 0, "");
							flag1=3;
							ultimos10();
						}
						
						if (strstr(message.text, "/Listar_archivos"))						//Si se recibe el comando /Listar_archivos, se reinicia la bandera 1
						{																	//y se llama a la función que lista los archivos
							ret = telebot_send_message(handle, message.chat->id, "Listado de archivos" , "HTML", false, false, 0, "");
							flag1=3;
							lista=-1;														//La bandera lista toma el valor -1 indicando la situación inicial
							ret = telebot_send_message(handle, message.chat->id, "Se listan los 10 archivos mas nuevos:\n", "HTML", false, false, 0, "");
							listar_archivos();
							ret = telebot_send_message(handle, message.chat->id, "Puede avanzar o retroceder con:\n/Anteriores\n/Posteriores\n", "HTML", false, false, 0, "");
						}
							if (strstr(message.text, "/Anteriores"))						//Si se recibe el comando /Anteriores, disminuye en 1 el valor de lista
							{																//y se llama a la función que lista los archivos
								lista--;
								listar_archivos();
							}
						
							if (strstr(message.text, "/Posteriores"))						//Si se recibe el comando /Anteriores, aumenta en 1 el valor de lista
							{																//y se llama a la función que lista los archivos
								lista++;
								number_file2=number_file2+19;								//El bucle for que obtiene el nombre de los archivos en listar_archivos finaliza 
																							//con el número actual de archivo menos 9, por lo que debemos sumar 19 para listar los 10 posteriores
								if(number_file2>lastfile&&lista==0)							//Si se supera el número del archivo más nuevo y lista=0, 
								{															//se debe seguir por el archivo más antiguo, sumarle 9 y levantar la bandera 2
									number_file2=firstfile+9;
									flag2=1;
								}
								
								if(number_file2>lastfile&&flag2==0)							//Si se supera el número del archivo más nuevo y flag2=0, estamos en el caso
								{															//en que ya se ha dado toda la vuelta al listado de archivos y debemos reiniciar el conteo de lista
									if(number_file2-19<lastfile)							//Esta condición contempla el caso de que el contador del número de archivo 
									{														//caiga entre los últimos 10 archivos
										lista=-1;
									}
								}
								
								if(number_file2>cantidad_archivos)							//Si se supera la cantidad máxima de archivos, number_file2 toma el
								{															//valor de la diferencia con la cantidad máxima, menos 1
									number_file2=number_file2-cantidad_archivos-1;
								}
								
								listar_archivos();
							}
						
						if (strstr(message.text, "/00"))									//Si el mensaje recibido contiene "/00", se debe enviar el archivo
						{																	//cuyo número envió el usuario
							strcat(cadena_main,message.text);								//Concatenación con el comando, la ruta de búsqueda y el número recibido
							strcat(cadena_main,"-*\" >/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt");						//Concatenación del arreglo resultante anterior con el archivo donde guardaremos el resultado
							system(cadena_main);											//Aplicación del comando de búsqueda
							
							file_main=fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/resultado.txt","r+");							//Apertura del archivo resultado.txt
							if(file_main==NULL)												//Si file_main=NULL, no pudo abrirse el archivo resultado.txt
							{
								printf("No se pudo abrir archivo\n");
								RetCode=-16;												//Si no pudo abrirse el archivo resultado.txt, RetCode toma el valor -16
							}
							fgets(name_file,300,file_main);									//Obtención de la ruta y el nombre del archivo
							fclose(file_main);												//Cierre del archivo resultado.txt
					
							enviar_archivo(name_file);										//Llamada a la función que envía el archivo, con la ruta y el nombre del archivo como argumento
							count=0;
						}   
				
					if (strstr(message.text, "/Parametros"))								//Si se recibe el comando /Parametros, la bandera 1 se coloca en alto
					{
						flag1=1;
					}
				   
						if (strstr(message.text, "/Mostrar_actuales"))						//Si se recibe el comando /Mostrar_actuales, se reinicia la bandera 1,
						{																	//se llama a la función que muestra los parámetros actuales y se levanta la bandera flag_parameters
							ret = telebot_send_message(handle, message.chat->id, "Se muestran los parametros actuales" , "HTML", false, false, 0, "");
							flag1=3;
							mostrar_actuales();
							ret = telebot_send_message(handle, message.chat->id, "\nDesea modificar alguno de los parametros:\n\n/Si\n\n/No\n" , "HTML", false, false, 0, "");
							flag_parameters=1;
						}
				
							if (strstr(message.text,"/Si") && flag_parameters==1)		//Si se recibe el comando /Si y además flag_parameters se encuentra en alto,
							{																//se llama a la función que lista los comandos para modificar parametros y se baja la bandera flag_parameters
									modificar_parametros();
									flag_parameters=0;
							}
					
							if (strstr(message.text,"/No") && flag_parameters==1)			//Si se recibe el comando /No y además flag_parameters se encuentra en alto,
							{																//no se realiza ninguna acción y se baja la bandera flag_parameters
								flag_parameters=0;
							}
				
						if (strstr(message.text, "/Modificar_parametros"))					//Si se recibe el comando /Modificar_parametros, se reinicia la bandera 1
						{																	//y se llama a la función que muestra la lista de parámetros al usuario
							  flag1=3;
							  modificar_parametros();
						}
                
							if (strstr(message.text, "/Canal_de_disparo"))					//Si se recibe el comando /Canal_de_disparo, se muestran los números de los canales
							{																//y se llama a la función modify_parameters
								ret = telebot_send_message(handle, message.chat->id,"Elija el canal de disparo:\n/0\n\n/1\n\n/2\n\n/3\n\n/4\n\n/5\n\n/6\n\n/7\n", "HTML", false, false, 0, "");
								flag_change=1;
								modify_parameters(flag_change);
							}
						
							if (strstr(message.text, "/Frecuencia_de_muestreo"))			//Si se recibe el comando /Frecuencia_de_muestreo, se muestran las frecuencias de muestreo 
							{																//seleccionables y se llama a la función modify_parameters
								ret = telebot_send_message(handle, message.chat->id,"Ingrese la frecuencia de muestreo en muestras por segundo:\n/30000\n\n/15000\n\n/7500\n\n/3750\n\n/2000\n\n/1000\n\n/500\n\n/100\n\n/60\n\n/50\n\n/30\n\n/25\n\n/15\n\n/10\n\n/5\n\n/2\n\n", "HTML", false, false, 0, "");
								flag_change=2;
								modify_parameters(flag_change);
							}
							
							if (strstr(message.text, "/Nivel_de_disparo"))					//Si se recibe el comando /Nivel_de_disparo, se muestra el formato del 
							{																//número a ingresar y se llama a la función modify_parameters
								ret = telebot_send_message(handle, message.chat->id,"Ingrese el nivel de disparo en el formato \"U.dc\". Por ejemplo, \"1.38\", en volts.\n", "HTML", false, false, 0, "");
								flag_change=3;
								modify_parameters(flag_change);
							}
							
							if (strstr(message.text, "/Muestras_post_trigger"))				//Si se recibe el comando /Muestras_post_trigger, se muestra la cantidad 
							{																//máxima a ingresar y se llama a la función modify_parameters
								ret = telebot_send_message(handle, message.chat->id,"Ingrese la cantidad de muestras post trigger. *Tenga en cuenta que la suma de la cantidad de muestras pre y post disparo no deben superar las 39000 muestras.*", "HTML", false, false, 0, "");
								flag_change=4;
								modify_parameters(flag_change);
							}
							
							if (strstr(message.text, "/Muestras_pre_trigger"))				//Si se recibe el comando /Muestras_pre_trigger, se muestra la cantidad 
							{																//máxima a ingresar y se llama a la función modify_parameters
								ret = telebot_send_message(handle, message.chat->id,"Ingrese la cantidad de muestras pre trigger. *Tenga en cuenta que la suma de la cantidad de muestras pre y post disparo no deben superar las 39000 muestras.*", "HTML", false, false, 0, "");
								flag_change=5;
								modify_parameters(flag_change);
							}
							
							if (strstr(message.text, "/Cantidad_archivos_almacenables"))	//Si se recibe el comando /Cantidad_archivos_almacenables, se muestra 
							{																//la consideración a tener en cuenta y se llama a la función modify_parameters
								ret = telebot_send_message(handle, message.chat->id,"Ingrese la cantidad de archivos almacenables. *Una vez alcanzada la cantidad especificada, los nuevos archivos sobrescribiran a los mas antiguos.* ", "HTML", false, false, 0, "");
								flag_change=6;
								modify_parameters(flag_change);
							}
					
								if (strstr(message.text, "/Aplicar_ahora"))					//Si se recibe el comando /Aplicar_ahora y la variable flag_change
								{															//se encuentra entre 0 y 7, indicando que se ha modificado algún parámetro:
									if(flag_change>0&&flag_change<7)
									{
										kill(pid_m,SIGUSR1);								//Se envía la señal SIGUSR1 al proceso cuyo PID recibimos a través de 
										flag_change=0;										//la FIFO, en este caso del programa de medición. La bandera flag_change toma el valor 0
									}
								}
								
								if (strstr(message.text, "/Tal_vez_luego"))					//Si se recibe el comando /Tal_vez_luego, simplemente se coloca en 0 la bandera flag_change 
								{
										flag_change=0;
								}
					
						switch(flag1)														//En base a la bandera 1, se muestran las opciones para la 
						{																	//descarga de archivos o para la modificación de parámetros
							case 0:
							{
								ret = telebot_send_message(handle, message.chat->id, "1:/Bajar_el_ultimo\n2:/Bajar_ultimos_10\n3:/Listar_archivos" , "HTML", false, false, 0, "");
							};
							break;
							case 1:
							{
								ret = telebot_send_message(handle, message.chat->id, "1:/Mostrar_actuales\n2:/Modificar_parametros" , "HTML", false, false, 0, "");
							};
							break;
						}
				
					if (strstr(message.text, "/Temp"))										//Si se recibe el comando /Temp, se aplica el comando que permite obtener la temperatura de la placa, 
					{																		//se lee la temperatura medida del archivo temperatura.txt y se envía al usuario
						ft = fopen("/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/temperatura.txt", "r+");
						system(comando);
						fgets(auxiliar,30,ft);
						ret = telebot_send_message(handle, message.chat->id, auxiliar, "HTML", false, false, updates[index].message.message_id, "");
						fclose(ft);
					}			
					
                if (strstr(message.text, "/dice"))											//Si se recibe el comando /dice, se envía al usuario una imagen descargada
                {
                    telebot_send_dice(handle, message.chat->id, false, 0, "");
                     char image[]={"/home/pi/Desktop/Archivos/telegrambot/telebot-master/test/test/download.jpeg"};
                    ret=telebot_send_photo(handle, message.chat->id,image,true,"","",false, updates[index].message.message_id,"");
                    if (ret != TELEBOT_ERROR_NONE)
                    {
                        printf("Failed to send photo: %d \n", ret);
                    }
                }
            }
            offset = updates[index].update_id + 1;								//Identificador único de la actualización. Los identificadores de actualización 
        }																		//comienzan con un cierto número positivo y aumentan secuencialmente.
		
		if (strstr(message.text, "/Exit"))										//Si se recibe el comando /Exit, se sale del bucle principal y se termina el programa
        {
            printf("\nSaliendo de Raspybot\n");
            break;
        }
        
        close(fifo_t);															//Cierre del descriptor de archivo de la FIFO
    }

    telebot_destroy(handle);													//Se destruye el manejador del bot

    return RetCode;
}
