/**
 * @file vibration_measurement.c
 * @author Fernando Guerrero
 * @date December, 2021
 * @base file mainTestAdda.c
 * @author Boris Bocquet <b.bocquet@akeoplus.com>
 * @date May, 2018
 * @brief Testing AD-DA-WS-RPI lib
 *
 * @details Usefull to perform Analog to Digital(ADC, using ADS1256 circuit) and Digital to Analog (DAC, using DAC8552).
 * @todo More code especially for DAC8552.
 */

// This Source Code Form is subject to the terms of the
// GNU Lesser General Public License V 3.0.
// If a copy of the GNU GPL was not distributed
// with this file, You can obtain one at https://www.gnu.org/licenses/lgpl-3.0.en.html

///////////////////////////////////Librerias////////////////////////////////////

#include "AD-DA-WS-RPI/AD-DA-WS-RPI.h"
#include <sys/time.h>  															// elapsed time to microseconds; measure sample rate
#include <time.h> 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

////////////////////////////Defines y Macros////////////////////////////////////

#define CS_ADC_0() printf("CS_ADC_0()\n")										//Define para iniciar la comunicación serie
#define CS_ADC_1() printf("CS_ADC_1()\n")										//Define para finalizar la comunicación serie
#define FIFO_PATH "/tmp/MI_FIFO"												//Nombre de la FIFO

/**
 * @brief Simple code to get the voltage on each 8 inputs (single ended) using the ADC1256 and write the input 1 value to output 1 using DAC8552.
 * You can use the jumper of the waveshare board to connect to 5V (power supply and vref), and connect ADO to ADJ (the potentiometer) and DAC to LEDA.
 * 
 * @return int 
 */

////////////////////////Declaración de variables globales///////////////////////

int canal;																		//Canal de disparo seleccionado 
int frequency;																	//Frecuencia de muestreo seleccionada
int N_post=0;																	//Cantidad de muestras post disparo seleccionada
int N_pre=0;																	//Cantidad de muestras pre disparo seleccionada
float nivel;																	//Nivel de disparo seleccionado
int cantidad_archivos;															//Cantidad máxima de archivos almacenables seleccionada
FILE *fp;																		//Descriptor de archivo que usaremos para el archivo que contiene los parámetros
FILE *file;																		//Descriptor de archivo que usaremos para el archivo que contiene las lecturas
int RetCode = 0;																//Variable de retorno para control
int flag_signal=0;																//Variable bandera: toma el valor 1 cuando llega una señal
struct timeval start,current;													//Estructuras donde guardaremos el tiempo
uint8_t buf[4];																	//Arreglo para la habilitación del buffer de entrada del ADC
char auxiliar[30]={""};															//Arreglo auxiliar utilizado en función parámetros
int DRATE_E;																	//Macro definida para la frecuencia de muestreo
int error=0;																	//Variable entera para control de errores

///////////////////////////////////FUNCIONES////////////////////////////////////

///////////////////////////////*Función "obtener"*//////////////////////////////							
int obtener()																	//Función que permite implementar la función atoi dentro de una sentencia condicional
{
	return(atoi(auxiliar));														//Función que convierte una cadena de caracteres en un número entero
}

//////////////////////////////*Función "frecuencia"*////////////////////////////

void frecuencia()																//Función que asigna a DRATE_E el valor correspondiente según 
{																				//el parámetro frecuencia obtenido del archivo de configuración
	switch(frequency)
	{
		case 30000:
		{DRATE_E=ADS1256_30000SPS;}													//La macro ADS1256_30000SPS al igual que las demás están definidas 
		break;																		//en la biblioteca AD-DA-WS-RPI.h y representan a cada frecuencia
		case 15000:
		{DRATE_E=ADS1256_15000SPS;}
		break;
		case 7500:
		{DRATE_E=ADS1256_7500SPS;}
		break;
		case 3750:
		{DRATE_E=ADS1256_3750SPS;}
		break;
		case 2000:
		{DRATE_E=ADS1256_2000SPS;}
		break;
		case 1000:
		{DRATE_E=ADS1256_1000SPS;}
		break;
		case 500:
		{DRATE_E=ADS1256_500SPS;}
		break;
		case 100:
		{DRATE_E=ADS1256_100SPS;}
		break;
		case 60:
		{DRATE_E=ADS1256_60SPS;}
		break;
		case 50:
		{DRATE_E=ADS1256_50SPS;}
		break;
		case 30:
		{DRATE_E=ADS1256_30SPS;}
		break;
		case 25:
		{DRATE_E=ADS1256_25SPS;}
		break;
		case 15:
		{DRATE_E=ADS1256_15SPS;}
		break;
		case 10:
		{DRATE_E=ADS1256_10SPS;}
		break;
		case 5:
		{DRATE_E=ADS1256_5SPS;}
		break;
		case 2:
		{DRATE_E=ADS1256_2d5SPS;}
		break;
	}
}

/////////////////////////////*Función "parametros"*/////////////////////////////

void parametros()																//Función que obtiene los parámetros guardados en el archivo de configuración
{
	char parameter;																//Caracter utilizado para la obtención del número de cada parámetro
	int i_parcial2=0;															//Entero auxiliar
	
	fp=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Parametros.txt","r");												//Abrimos el archivo de configuración
	if(fp==NULL)																//Si fp=NULL no pudo abrirse el archivo Parametros.txt
	{
		printf("No se pudo abrir archivo Parametros.txt\n");
		fp=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Parametros_respaldo.txt","r");								//Abrimos el archivo de configuración de respaldo
		error=-1;																//Si no se puede abrir el archivo Parametros.txt
		if(fp==NULL)															
		{
			printf("No se pudo abrir archivo Parametros_respaldo.txt\n");
			error=-2;															//Si no se puede abrir el archivo Parametros_respaldo.txt
		}
	}
	
	while(!feof(fp))															//Mientras no se llegue hasta el final del archivo se ejecuta este loop
	{
		parameter=fgetc(fp);													//Obtenemos el primer caracter de cada fila
		fgets(auxiliar,30,fp);													//Obtenemos la oración restante
		i_parcial2=0;
		switch(parameter)														//Según el valor del primer caracter de cada fila es el parámetro a obtener
		{
			case '1':															//Parámetro canal
			{	
				auxiliar[0]=auxiliar[7];										//El número de canal se encuentra en la posición número 7 de la oración restante
				canal=obtener();												//Obtenemos el valor numérico del canal
				if(canal>7)
				{canal=7;}
				if(canal<0)
				{canal=0;}   
			}
			break;
			case '2': 															//Parámetro frecuencia
			{	
				auxiliar[5]='\0';												//Hasta de la posición 5 borramos cualquier contenido residual
				while(auxiliar[12+i_parcial2]!='\0')							//A partir de la posición 12 obtenemos la frecuencia, hasta el final del arreglo
				{
					auxiliar[i_parcial2]=auxiliar[12+i_parcial2];				//Guardamos los caracteres de la frecuencia en las primeras posiciones del arreglo auxiliar	
					i_parcial2++;
				}
				frequency=obtener();											//Obtenemos el valor numérico de la frecuencia
				frecuencia();													//Llamamos a la función frecuencia con la cual asignamos a DRATE_E la macro correspondiente
			}
			break;
			case '3':															//Parámetro nivel de disparo
			{
				auxiliar[3]='\0';												//Hasta de la posición 3 borramos cualquier contenido residual
				auxiliar[0]=auxiliar[7];										//Guardamos los caracteres de la frecuencia en las primeras posiciones 	
				auxiliar[1]=auxiliar[9];										//del arreglo auxiliar, sin contar el punto decimal
				auxiliar[2]=auxiliar[10];
				nivel=obtener()/100.0;											//Al dividir por 100 obtenemos el nivel de tensión de disparo con dos posiciones decimales
			}
			break;
			case '4':															//Parámetro cantidad de muestras post trigger
			{
				while(auxiliar[22+i_parcial2]!='\0')							//A partir de la posición 22 obtenemos la cantidad de muestras post disparo, hasta el final del arreglo
				{
					auxiliar[i_parcial2]=auxiliar[22+i_parcial2];				//Guardamos los caracteres de la cantidad de muestras post disparo 
					i_parcial2++;												//en las primeras posiciones del arreglo auxiliar
				}
				N_post=obtener();												//Obtenemos el valor numérico de la cantidad de muestras post disparo
				if((N_pre+N_post)>39000)										//Limitamos la cantidad máxima de muestras a guardar a 39000 que es el tamaño máximo del buffer
				{	
					N_pre=18000;												//Tomamos un margen de precaución asignando 36000 muestras en total
					N_post=18000;
				}
			}
			break;
			case '5': 															//Parámetro cantidad de muestras pre trigger
			{
				while(auxiliar[21+i_parcial2]!='\0')							//A partir de la posición 21 obtenemos la cantidad de muestras pre disparo, hasta el final del arreglo
				{
					auxiliar[i_parcial2]=auxiliar[21+i_parcial2];				//Guardamos los caracteres de la cantidad de muestras post disparo 
					i_parcial2++;												//en las primeras posiciones del arreglo auxiliar
				}
				N_pre=obtener();												//Obtenemos el valor numérico de la cantidad de muestras pre disparo
				if((N_pre+N_post)>39000)										//Limitamos la cantidad máxima de muestras a guardar a 39000 que es el tamaño máximo del buffer
				{
					N_pre=18000;												//Tomamos un margen de precaución asignando 36000 muestras en total
					N_post=18000;
				}
			}
			break;
			case '6': 															//Parámetro cantidad de archivos almacenables
			{
				while(auxiliar[22+i_parcial2]!='\0')							//A partir de la posición 22 obtenemos la cantidad de archivos almacenables, hasta el final del arreglo
				{
					auxiliar[i_parcial2]=auxiliar[22+i_parcial2];				//Guardamos los caracteres de la cantidad de archivos almacenables
					i_parcial2++;												//en las primeras posiciones del arreglo auxiliar
				}
				cantidad_archivos=obtener();									//Obtenemos el valor numérico de la cantidad de archivos almacenables
			}
			break;
		}
	}
	fclose(fp);																	//Cerramos el descriptor del archivo Parámetros.txt
}

///////////////////////////////*Función "archivo"*//////////////////////////////

int archivo()																	//Función para obtener el nombre del archivo a generar
{
	int number_file;															//Entero con el número del archivo a generar
	int largo2=0;																//Entero auxiliar con la longitud del arreglo obtenido
	int flag_last=0;															//Variable bandera que toma el valor 1 cuando se alcanza la cantidad máxima de archivos almacenables
	FILE *fc;																	//Descriptor para el archivo con el número del archivo a generar
	char next_file[40];															//Arreglo auxiliar donde guardaremos el nombre del siguiente archivo a generar
	char current_time[40];														//Arreglo auxiliar donde guardaremos la fecha actual de generación del archivo
	char cadena1[400]={"rm -f ./home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/*"};													//Arreglo con sintaxis inicial del comando rm
	char end_route[]={"/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/Muestras/"};
	
	fc=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/lastfile.txt","r+");												//Abrimos el archivo en modo lectura/escritura que contiene el número del último archivo creado o modificado
	if(fc==NULL)																//Si fc=NULL no pudo abrirse el archivo lastfile.txt
	{
		error=-3;																//No pudo abrirse el archivo lastfile.txt en modo lectura
		fc=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/lastfile.txt","w+");											//En caso de no existir el archivo, se crea y luego se abre en modo escritura
		if(fc==NULL)															//Si fc=NULL no pudo crearse el archivo lastfile.txt
		{printf("Error al crear el archivo\n");
		 error=-4; 																//No pudo abrirse el archivo lastfile.txt en modo escritura
		 return error;}
	}
	
	fgets(auxiliar,40,fc);														//Leemos el número del archivo a generar
	largo2=strlen(auxiliar);													//Obtenemos la longitud del arreglo auxiliar
	
	number_file=obtener();														//Guardamos en number_file el número de archivo
	sprintf(next_file,"00%d",number_file);										//Guardamos el número de archivo en formato string en next_file
	
	gettimeofday(&start, NULL); 												//Obtenemos la fecha y hora actual en la estructura start
	strftime(current_time,30,"%d|%m|%y-%H:%M:%S",localtime(&start.tv_sec));		//Guardamos la fecha y hora actual en current_time en el formato especificado entre comillas
	
	strcat(cadena1,next_file);													//Concatenamos cadena1 con el número del archivo a generar
	strcat(cadena1,"-*");														
	int Sys=system(cadena1);													//Con la función system escribimos por línea de comando el string contenido en cadena1
	if (Sys != 0)																//Si Sys es distinto de cero, no se encontró y por lo tanto no se eliminó ningún archivo con el nombre especificado
	{																			
		RetCode = -1;
	}
		
	strcat(next_file,"-");														//Conformamos el nombre del archivo concatenando el número, la fecha y hora y el formato del mismo
	strcat(next_file,current_time);
	strcat(next_file,".txt");
	
	fseek(fc,-largo2,SEEK_CUR);													//Reubicamos el puntero del archivo al inicio del mismo 
	
	if(number_file==cantidad_archivos)											//Si el número del último archivo coincide con el máximo especificado por el usuario, reiniciamos el conteo 
	{
		number_file=0;
		system("rm -f ./home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/*lastfile*");												//Volvemos a crear el archivo lastfile.txt reiniciando el conteo del número de archivo 
		fc=fopen("/home/pi/Desktop/Archivos/libreria/RaspberryPi-ADC-DAC/build/lastfile.txt","w+");	
		fprintf(fc,"%d",number_file);
		flag_last=1;															//Levantamos la bandera flag_last
	}
	else                                                                        //Si no hemos llegado a la máxima cantidad de archivos simplemente sumamos 1 al número del último archivo
	{
		number_file++;
		fseek(fc,0,SEEK_SET);													//Reubicamos el puntero del archivo al inicio del mismo
		fprintf(fc,"%d",number_file);											//Escribimos en lastfile.txt el número del siguiente archivo a generar (no el actual)
	}		
	strcat(end_route,next_file);												//Concatenación de la ruta de creación del archivo con el nombre de este
	file=fopen(end_route,"a+");													//Abrimos el archivo donde guardaremos las lecturas. Si el archivo no existe se crea
	fclose(fc);																	//Cerramos el archivo lastfile.txt
	
	gettimeofday(&start, NULL);  												//Función para obtener la fecha y la hora actual  
	fprintf(file,"%s\n",asctime(localtime(&start.tv_sec)));						//Imprimimos la fecha y la hora actual en el archivo generado
	if(flag_last==1)															//Si flag_last=1 el encabezado del archivo debe poseer el número de la cantidad máxima de archivos (esto se debe a que anteriormente reiniciamos la cuenta)
	{
		fprintf(file,"Parámetros:\nCanal de disparo:	%d\nFrecuencia de muestreo:	%d\nNivel de disparo=	%f\nCantidad de muestras post trigger =	%d\nCantidad de muestras pre trigger =	%d\nNúmero de archivo =	%d\n",canal,frequency,nivel,N_post,N_pre,cantidad_archivos);
	}	
	else																		//Si flag_last=0 el encabezado del archivo debe poseer el número del archivo actual que es igual a number_file-1
	{
		fprintf(file,"Parámetros:\nCanal de disparo:	%d\nFrecuencia de muestreo:	%d\nNivel de disparo=	%f\nCantidad de muestras post trigger =	%d\nCantidad de muestras pre trigger =	%d\nNúmero de archivo =	%d\n",canal,frequency,nivel,N_post,N_pre,number_file-1);
	}	
	fprintf(file,"	Microvolts/cuenta =	5000000/8388608\nCanal 0 (uV)	Canal 1 (uV)	Canal 2 (uV)	Canal 3 (uV)	Canal 4 (uV)	Canal 5 (uV)	Canal 6 (uV)	Canal 7 (uV)	Tiempo: canal 0 (S) +	uS\r\n"); 	//Encabezado de columnas para cada canal 
	
	return 0;
}

///////////////////////////////*Manejador de señal*/////////////////////////////

void signal_handler(int unusable)												//Manejador de la señal proveniente del programa de Telegram
{
	unusable=0;																	//Variable sin uso necesaria para el correcto funcionamiento del manejador
	printf("\nunusable=%d\n",unusable);
	flag_signal=1;																//Levantamos la bandera flag_signal indicando que llegó una señal
}

///////////////////////////////*Función "lectura"*//////////////////////////////

int lectura(int NbChannels, int AdcValues[][8], ADS1256_SCAN_MODE mode, int loop)	//Función de lectura implementada con matrices y no arreglos dinámicos
{
	
	int i;
	for (i = 0; i < NbChannels; i++)												//Con el for recorremos los 8 canales
	{
		
		ADS1256_WaitDRDY_LOW();														//Espera a que el pin DRDY se ponga en bajo lo que indica que  
		                                                                            //la conversión anterior terminó y puede iniciar la nueva conversión
		uint8_t CurChannel = i;
		
		if (mode == SINGLE_ENDED_INPUTS_8)											//Modo de 8 entradas individuales
			ADS1256_SetChannel(CurChannel);											//Seteamos el canal 
		else
			ADS1256_SetDiffChannel(CurChannel);
		
		bsp_DelayUS(MASTER_CLOCK_PERIOD_USEC_TIMES_24);								//Tiempos de espera necesarios para que se establezcan correctamente los parámetros
		
		ADS1256_WriteCmd(CMD_SYNC);													//Comando de sincronismo
		bsp_DelayUS(MASTER_CLOCK_PERIOD_USEC_TIMES_24);
		
		ADS1256_WriteCmd(CMD_WAKEUP);												//Junto con CMD_SYNC son comandos necesarios antes realizar la medición
		bsp_DelayUS(MASTER_CLOCK_PERIOD_USEC_TIMES_24);
		
		
		AdcValues[loop][i] = ADS1256_ReadData();									//Lee la entrada seleccionada y almacena la muestra en una posicion del
																					//arreglo AdcValues
	}
	
	return 0;
}

////////////////////////////////*Función "main"*////////////////////////////////

int main(void)
{
	signal(SIGUSR1, signal_handler);											//Si llega una señal SIGUSR1, se atiende en el manejador de señal signal_handler

//////////////////////////DEFINICIÓN DE VARIABLES///////////////////////////////
	
	int NChannels = 8;															//Cantidad de canales a leer
	int MainLoop = 0;															//Variable a usar en caso de que no deseemos que el programa se ejecute de forma indefinida sino n veces																
	int j_general; 																//Entero auxiliar
	int flag_triggered=0;														//Variable bandera que toma el valor 1 cuando la medición supera el valor umbral 
	int i_referencia;															//Entero auxiliar que usaremos de referencia al guardar las muestras en el archivo
	int pid_m;																	//Entero donde obtendremos el PID del proceso
	int err;																	//Entero de retorno al crear la FIFO
	int fifo_d;																	//Descriptor de la FIFO abierta
	int level_triggered=0;														//Variable donde guardaremos la combinación equivalente al nivel seleccionado
	int N_total=0;																//N será igual a la cantidad total de muestras a guardar en el archivo, por lo tanto, N=post+pre
	int Init;																	//Variable de retorno al iniciar el ADC
	int Id = 0;																	//Variable donde se guarda el identificador de la placa
	int i_general=0;															//Entero auxiliar
	int i_parcial=-1;															//Entero auxiliar (se declara en -1 para tener en cuenta la muestra que produjo el disparo)
	char mypid[20];																//Arreglo auxiliar con el PID del proceso a enviar por la FIFO
	
//////////////////////////////INICIO DE PRUEBA//////////////////////////////////
	
	printf("Iniciando prueba\r\n");
	
	int initSpi = spi_init();													//Inicializamos la comunicación SPI
	if (initSpi != 1)															//Si initSpi es distinto de 1, no se pudo iniciar la comunicación serie con la placa
	{
		printf("SPI init failed with code %d\r\n", initSpi);
		error=-5;																//No pudo iniciarse la comunicación serie
		return error;
	}
	printf("SPI initialized\r\n");
	
	parametros();																//Obtenemos los parámetros
	
	level_triggered=nivel*8388608/5;											//Conversion de volts a numero combinacional del nivel de disparo
	N_total=N_pre+N_post;														//La cantidad total de muestras a almacenar
	
	pid_m=getpid();																//Obtención del PID del proceso
	sprintf(mypid,"%d",pid_m);													//Conversión a arreglo de caracteres del PID del proceso
	
	err = mkfifo(FIFO_PATH,0777);												//Creamos la FIFO con los permisos especificados en el segundo argumento
	if(err == -1) 																//Si la FIFO ya existe porque no pudo eliminarse, err toma el valor -1
	{printf("\nError al crear FIFO, la FIFO ya existe\n");
	 RetCode=-2;}
	
	fifo_d = open(FIFO_PATH,O_RDWR | O_NONBLOCK,0); 							//Abrimos la FIFO en modo lectura/escritura. Debe ser no bloqueante
	if(fifo_d == -1)															//Si open devuelve -1 se produjo un error al abrir la FIFO, 
	{printf("\nError al abrir FIFO\n");											//caso contario devuelve el descriptor de archivo de la FIFO
	 RetCode=-3;}										
	
	err = write(fifo_d, mypid, sizeof(mypid));									//Escribimos en la FIFO el PID del proceso
	if(err == -1)																//Si err=-1 se produjo un error al escribir en la FIFO
	{printf("\nError al escribir en FIFO\n");
	 RetCode=-4;}

	
///////////////////////////CONFIGURACIÓN DEL ADC////////////////////////////////
	
	Init = ADC_DAC_Init(&Id, ADS1256_GAIN_1, DRATE_E);							//Inicializamos el ADC. En Id obtenemos el numero de identificación del chip que debería ser igual a 3.
	if (Init != 0)																//Elegimos ganancia igual a 1 y la frecuencia de muestreo en muestras/segundo
	{
		error = -6;															    //Si Init es distinto de 0, no pudo iniciarse el ADC y retornamos el valor -6.
		return error;
	}
	printf("ADC_DAC_Init\r\n");
/////////////////////Habilitación del buffer de entrada/////////////////////////Comentar las líneas resaltadas si desea deshabilitar el buffer de entrada 
	buf[0] = (0 << 3) | (1 << 2) | (1 << 1);
	buf[1] = 0x08;
	buf[2] = (0 << 5) | (0 << 3) | ((uint8_t) ADS1256_GAIN_1 << 0);
	CS_ADC_0();																	/* SPI  cs = 0 */
	ADS1256_Send8Bit(CMD_WREG | 0); 											/* Write command register, send the register address */
	ADS1256_Send8Bit(0x03);														/* Register number 4,Initialize the number  -1*/
	ADS1256_Send8Bit(buf[0]); 													/* Set the status register */
	ADS1256_Send8Bit(buf[1]); 													/* Set the input channel parameters */
	ADS1256_Send8Bit(buf[2]); 													/* Set the ADCON control register,gain */
	ADS1256_Send8Bit(buf[3]); 													/* Set the output rate */
	
	CS_ADC_1(); 																/* SPI  cs = 1 */
	
	printf("init done !\r\n");

//////////////////////////////BUCLE PRINCIPAL///////////////////////////////////
	while (1 == 1)
	{
		
		int64_t tiempo[40000][2]={};											//Matriz donde guardaremos el tiempo en segundos y microsegundos
		int32_t AdcValues[40000][8]={};											//Arreglo donde guardaremos los valores leidos de cada canal(se rellena con los argumentos pasados en ReadAdcValues)
			
//////////////////////////////BUCLE SECUNDARIO//////////////////////////////////
		while(1)
		{	
			if(flag_signal==1)																//Si flag_signal=1, llegó una señal y deben aplicarse los nuevos parámetros
			{
				 parametros();																//Se obtienen los nuevos parámetros
				 level_triggered=nivel*8388608/5;											//Conversion de volts a numero combinacional 
				 N_total=N_pre+N_post;														//Obtención del total de muestras a almacenar
				 Init = ADC_DAC_Init(&Id, ADS1256_GAIN_1, DRATE_E);							//Inicializamos el ADC. En Id obtenemos el numero de identificación del chip que debería ser igual a 3.
				 if (Init != 0)														 		//Elegimos ganancia igual a 1 y la frecuencia de muestreo en muestras/segundo
				 {
					RetCode = -5;															//Si no pudo reiniciarse el ADC, el valor de retorno valdrá -5
				 }
				 i_general=0;																//Reinicio del conteo general del buffer
				 i_parcial=-1;																//Reinicio del conteo parcial
				 flag_signal=0;																//Bajada de la bandera flag_signal
			}
			
			gettimeofday(&current, NULL);      												//Obtención de el tiempo actual en la estructura current
			tiempo[i_general][0]=current.tv_sec;											//Guardamos los segundos en la primera columna y la fila especificada por i_general 
			tiempo[i_general][1]=current.tv_usec;											////Guardamos los microsegundos en la segunda columna y la fila especificada por i_general
		
			lectura(NChannels,AdcValues,SINGLE_ENDED_INPUTS_8,i_general);					//Pasamos el arreglo con los canales a leer, la cant. de canales, 
																							//el modo (singular o diferencial) y el arreglo donde guardar los valores leidos
			if(AdcValues[i_general][canal]>=level_triggered)								//Si la medición es mayor o igual al nivel especificado, levantamos la bandera flag_triggered
			{
				flag_triggered=1;
			}
			if(flag_triggered==1)															//Si la bandera flag_triggered=1, incrementamos el contador parcial
			{
				i_parcial++;
			}
			if(i_parcial==N_post+1)															//Si el contador parcial alcanza la cantidad de muestras post disparo + 1, salimos del bucle de medición
			{		
				break;
			}
			if(i_general==38999)															//Si se alcanza la última posición del buffer, reiniciamos el contador general
			{
				i_general=0;
			}
			else																			//Caso contrario, incrementamos en 1 la posición del buffer
			{
				i_general++;
			}
		}
		
		archivo();																			//Obtenemos y abrimos el archivo donde guardaremos las mediciones
		
		i_referencia=i_general-N_total;														//Generación de la variable que tomaremos de referencia

		if(i_referencia<0)																	//Si i_referencia es negativo, estamos en el caso en el que las muestras se encuentran tanto al final como al principio del buffer
		{
			for(j_general=38999+i_referencia;j_general<39000;j_general++)											//Recorremos el buffer desde la posición final de este menos i_referencia hasta la posición final
			{
				fprintf(file,"%d	%d	%d	%d	%d	%d	%d	%d	%ld	%ld\r\n",AdcValues[j_general][0], AdcValues[j_general][1], AdcValues[j_general][2], AdcValues[j_general][3], AdcValues[j_general][4], AdcValues[j_general][5],
				AdcValues[j_general][6], AdcValues[j_general][7], tiempo[j_general][0], tiempo[j_general][1]); 		//Guardamos en el archivo la muestra obtenida de cada canal con su estampa de tiempo en segundos y microsegundos
			}
			for(j_general=0;j_general<i_general;j_general++)														//Recorremos el buffer desde la posición inicial hasta la posición con la última muestra tomada
			{
				fprintf(file,"%d	%d	%d	%d	%d	%d	%d	%d	%ld	%ld\r\n",AdcValues[j_general][0], AdcValues[j_general][1], AdcValues[j_general][2], AdcValues[j_general][3], AdcValues[j_general][4], AdcValues[j_general][5],
				AdcValues[j_general][6], AdcValues[j_general][7], tiempo[j_general][0], tiempo[j_general][1]); 		//Guardamos en el archivo la muestra obtenida de cada canal con su estampa de tiempo en segundos y microsegundos
			}
		}
		else																				//Si i_referencia es positivo, estamos en el caso en el que las muestras se encuentran en la zona intermedia del buffer
		{
			for(j_general=i_referencia;j_general<i_referencia+N_total+1;j_general++)								//Recorremos el buffer desde i_referencia (1ra muestra pre disparo), 
			{																										//hasta la última muestra tomada (última muestra post disparo)
				fprintf(file,"%d	%d	%d	%d	%d	%d	%d	%d	%ld	%ld\r\n",AdcValues[j_general][0], AdcValues[j_general][1], AdcValues[j_general][2], AdcValues[j_general][3], AdcValues[j_general][4], AdcValues[j_general][5],
				AdcValues[j_general][6], AdcValues[j_general][7], tiempo[j_general][0], tiempo[j_general][1]); 		////Guardamos en el archivo la muestra obtenida de cada canal con su estampa de tiempo en segundos y microsegundos
			}
		}
		
		fclose(file);																		//Cerramos el descriptor de archivo

		flag_triggered=0;																	//Bajamos la bandera flag_triggered
		i_parcial=-1;																		//Reiniciamos el contador parcial
		i_general=0;																		//Reiniciamos el contador general
		
		MainLoop++;																			//Incrementamos el contador del bucle principal
		/*This loop proves that you can close and re-init pacefully the librairie. Prove it several times (e.g. 3) and then finish the code.*/
		//if (MainLoop == 1000)																//Descomentar estas dos lineas si desea que el bucle se ejecute 
		//	break;																			//una determinada cantidad de veces en lugar de en forma indefinida
		
	}
		close(fifo_d);																		//Cerramos el descriptor de archivo de la FIFO
	
		printf("ADC_DAC_Close\r\n");
		int CloseCode = ADC_DAC_Close();													//Finalizamos la comunicación SPI y ponemos al conversor en standby
		if (CloseCode != 0)
		{
			error = -7;																	   //Si la función anterior falla, retornamos el valor -7
		}
	printf("Test ADDA finished with returned code %d and error %d\r\n", RetCode, error);
	
	return error;																			//Retorno de la variable de control error
}
