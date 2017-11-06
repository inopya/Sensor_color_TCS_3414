/*
 #       _\|/_   
 #       (O-O)      A ver, que tenemos por aqui....
 # ---oOO-(_)-OOo------------------------------------

 ####################################################
 # ************************************************ #
 # *      Lector de COLORES  con  TCS 3414TC      * #
 # *                                              * #
 # *         Autor: Eulogio López Cayuela         * #
 # *       Versión 3.0     Fecha: 18/10/2017      * #
 # *                                              * #
 # ************************************************ #
 ####################################################


  NOTA SOBRE VERSIONES PREVIAS:
    - v1.0 Acceso al sensor teniendo que programar los parametros en el codigo para cada prueba.
    
    - v2.0 Uso de un menu (heredado de la centrifugadora) para introducir parametros en tiempo de ejecucion.
      Para acceder al menu de seleccion de parametros hay que realizar una pulsacion larga.
      Para corregir un dato ya introducido (volver a un submenu anterior) hay que realizar una pulsacion larga.
      Uso de un zumbador (opcional) para emitir señales sonoras avisando de pulsaciones y eventos).

  NOTAS DE ESTA VERSION:
    - v3.0 (ESTA VERSION) Añadida pantalla nokia de Javier de modo que pueda usarlo tal cual.
      No añado (por ahora) el soporte para la SD.
      No he manejado nunca ni la LCD de Nokia ni el lector de SD, asi que iremos por partes.

    - Control de sensor de color TCS3414 mediante ARDUINO UNO.
    - Version completa, funcional y ESTABLE.
    - Disponemos de dos modos de lectura, con los parametros manueales establecidos, 
      o un modo autocompensado en el que las componentes se obtiene  de tres lecturas consecutivas.
      Este modo se activa desde el menu de seleccion de parametros haciendo 2 pulsaciones largas seguidas.
    
    # Para el manejo de registros de configuracion usamos 4+1 funciones basicas:
      * TSC3414_begin():
        - Para inicializar el sensor y activar su ACD
        
      * TSC3414_set_Timing_Register (int milisegundos):
        - Nos permite seleccionar entre tiempos de muestreo predefinidos (12, 100 y 400 ms)
        
      * TSC3414_set_Manual_Timing (int milisegundos):
        - Nos permite seleccionar manualmente el tiempo de muestreo (en milisegundos)
          (tras una integracion manual, el ADC ¡queda desactivado!
          
      * TSC3414_set_gain_and_prescaler (byte gain, byte prescaler)
        - Permite seleccionar la ganacia y el preescalado.    
      
      * colorRead(int canalColor)
        - Permite acceder directamente al registro de color del chip que se le indica en el parametro.

    # Para la obtencion de colores usamos 2 funciones:
      * detectarColor()
        - funcion base que accede a los registros de color con los parametros establecidos
          y que se pueden modificar en cualquier momento durante la ejecucion.
          Devuelve un dato de tipo struct (personalizado), para poder retornar varios datos, sin usar punteros.
          
      * detectarSoloComponentes()
        - Este viene a ser un modo de lectura 'autocompensado'
          Como que no consigo demasiada concordancia con la realidad, en esta funcion
          hago la lectura de cada componente con parametros distintos y que estan prefijados
          dentro de la funcion (no se pueden cambiar en tiempo de ejecucion).
          Devuelve un dato de tipo struct (personalizado), que contiene las tres componentes del color.
  
    - Para procesar a una muestra se ha de activar el pulsador
    
    - Menu en LCD NOKIA para modificar los parametros del sensor.
    
    ** Otras explicaciones sobre el funcionamiento de funciones y sus parametros, en el codigo de dichas funciones
  
 */


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION PARA IMPORTAR BIBLIOTECAS
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/


//------------------------------------------------------
//Importacion de las biblioteca necesarias
//------------------------------------------------------
#include <Wire.h>                 // Biblioteca para comunicaciones I2C
#include <LCD5110_Basic.h>        // Librería pantalla Nokia

LCD5110 myGLCD(3,4,5,6,7);        // recordar...  Resolución: 84×48
extern uint8_t SmallFont[];
//extern uint8_t MediumNumbers[];
//extern uint8_t BigNumbers[];


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION DE DECLARACION DE CONSTANTES  Y  VARIABLES GLOBALES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//------------------------------------------------------
// Creamos las instancia de los objetos:
//------------------------------------------------------

// ****  Definiciones relacionadas con el pulsador de seleccion  ****
#define PIN_PULSADOR_SELECCION  8   // Patilla para el sensor tactil (o un pulsador normal)

// ****  Definiciones relacionadas con el motor  ****
#define POTENCIOMETRO  A0           // potenciometro conectado en Analogica 0
                                    // se usara en conbinacion con el pulsador 
                                    // para navegar por los menus e introducir valores          


// ****  Definiciones relacionadas con el altavoz  ****
#define PIN_ZUMBADOR        9       //patilla para el altavocillo
#define FRECUENCIA       1800       //frecuencia (Hz)del tono. Entre 2000 y 2100, muy cabrones y buenos para ALARMA
#define TIEMPO_SONIDO     200       //durarion del tono (milisegundos)
#define TIEMPO_SILENCIO   150       //durarion del silencio (milisegundos)


/* 
 * definimos una nuevo tipo de datos "SensorData_type"  (de tipo struct) 
 * para contener los datos devueltos por el sensor como un bloque unico
 */
struct SensorData_type  {
                          unsigned int w; 
                          unsigned int r; 
                          unsigned int g;
                          unsigned int b;
                        };


#define SENSOR_COLOR_ADDR    0x39   // Direccion I2C del sensor TCS3414TC
#define PIN_LED_SENSOR       11     // pin para la luz de apoyo del sensor de color (algunos modeles)


/* se usaran para contener los datos programados mediante el menu */
int tiempo_integracion_sensor = 300;
byte ganancia_sensor = 16;
byte prescaler_sensor = 1;

/* define el modo de operacion */
boolean FLAG_color_compensado = false; //define el modo de operacion. Parametros manuales(false), o Autocompensado(true)

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION OPERATIVA DEL PROGRAMA
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/


//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
// FUNCION DE CONFIGURACION
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 

void setup() //inicializacion del sistema
{
  Serial.begin(9600);           // inicializar el puerto serial para 'DEBUG'

  myGLCD.InitLCD(65); //inicio pantalla nokia
  myGLCD.setFont(SmallFont);    

  /* ------ MostarVersion ------------------------------------------ */
  myGLCD.print("Sensor TSC3414 v3.0", LEFT, 0);
  myGLCD.print("Preparado...", RIGHT, 8);
  myGLCD.print("Pulsa OK para" , CENTER, 24);
  myGLCD.print("tomar muestra" , CENTER, 32);
  /* ------ FIN MostarVersion -------------------------------------- */  

  pinMode(PIN_PULSADOR_SELECCION, INPUT);
  pinMode(PIN_LED_SENSOR, OUTPUT);
  //digitalWrite(PIN_LED_SENSOR, true);  //apagar
  digitalWrite(PIN_LED_SENSOR, false); //encender
  


    
 
  Wire.begin();
  TSC3414_begin();

  //parametros por defecto (en estas funciones solo podemos poner valores preestablecidos)
  TSC3414_set_Timing_Register (400);           // int milisegundos (12,100,400) 
  TSC3414_set_gain_and_prescaler (64, 4);      // byte gain(1,4,16,64), byte prescaler(1,2,4,8,16,32,64)  
  pitidos(2);                                 // dos pitidos para avisar de que se ha inicializado todo
 
  delay(100);
}



//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//  BUCLE PRINCIPAL DEL PROGRAMA   (SISTEMA VEGETATIVO)
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 


void loop()
{

  boolean FLAG_pulsador = false;
  byte pulsacion = leer_Pulsador_Tactil();  // leemos el estado del pulsador tactil

  if (pulsacion == 1){                      //condicion para tomar una lectura de color
    myGLCD.clrScr();
    myGLCD.print("Muestreando...", LEFT, 0);

    pulsacion = 0;                          //anulamos el estado de la pulsacion por precaucion
    FLAG_pulsador = true;                   //condicion apra tomar una lectura de color
  }
    
  if (pulsacion == 2){                      //condicion para programar nuevos parametros
    pulsacion = 0;                          //anulamos el estado de la pulsacion por precaucion
    programar_Parametros();                 //volvemos al menu de programar parametros
    FLAG_pulsador = true;                   //forzamos la toma de una muestra 
                  
    //establecer ganancia y prescales hasta nuevo cambio 
    TSC3414_set_gain_and_prescaler (ganancia_sensor, prescaler_sensor);  
  }
  
  if (FLAG_pulsador == true) {
    FLAG_pulsador = false;                        //anulamos el estado por precaucion
    struct SensorData_type  muestra_W_RGB;        // creamos una variable para recibir la informacion de color
     
    if (FLAG_color_compensado == false){          //modo de parametros manuales (segun menu)
      //hay establecer el tiempo de muestreo en cada medida, ya que operamos en manual
      TSC3414_set_Manual_Timing (tiempo_integracion_sensor);
      //digitalWrite(PIN_LED_SENSOR, false);      //encender luz si es que existe la opcion
      //delay(10); para estabilizar la luz
      muestra_W_RGB = detectarColor();            //leer los registros de color
    }

    if (FLAG_color_compensado == true){           //modo de funcionamiento Autocompensado
      muestra_W_RGB = detectarSoloComponentes();
    }
    
    unsigned int blanco = muestra_W_RGB.w >> 8;   //convertir los valores del sensor de 16 bits a 8 bits
    unsigned int rojo   = muestra_W_RGB.r >> 8;
    unsigned int verde  = muestra_W_RGB.g >> 8;
    unsigned int azul   = muestra_W_RGB.b >> 8;
  
  /*
    //Mostrar los resultados por SERIAL
    Serial.print(tiempo_integracion_sensor);Serial.print(", ");
    Serial.print(ganancia_sensor);Serial.print(", ");
    Serial.println(prescaler_sensor);  
       
    Serial.print(muestra_W_RGB.w);
    Serial.print(">> \t blanco 8 bits: ");Serial.println(blanco);
    Serial.print(muestra_W_RGB.r);
    Serial.print(">> \t rojo 8 bits: ");Serial.println(rojo);
    Serial.print(muestra_W_RGB.g);
    Serial.print(">> \t verde 8 bits: ");Serial.println(verde);
    Serial.print(muestra_W_RGB.b);
    Serial.print(">> \t azul 8 bits: ");Serial.println(azul);

    Serial.println("-----------------------");
  */
  

    //mostrar los resultados en el LCD NOKIA
    myGLCD.clrScr();
    if (FLAG_color_compensado == true){
      myGLCD.print("-COMPENSADO-" , CENTER, 0);
    }
    else{
      myGLCD.printNumI(tiempo_integracion_sensor, LEFT, 0); myGLCD.print("," , 24, 0);  // LEFT = 0
      myGLCD.printNumI(ganancia_sensor ,  36, 0); myGLCD.print("," , 48, 0);
      myGLCD.printNumI(prescaler_sensor , 60, 0);
    }

    myGLCD.print("LUZ >> ", 0, 16); myGLCD.printNumI(blanco,  50, 16, 3, ' ');
    myGLCD.print("rojo:  " , LEFT, 24); myGLCD.printNumI(rojo  , 50, 24, 3, ' ');
    myGLCD.print("verde: " , LEFT, 32); myGLCD.printNumI(verde , 50, 32, 3, ' ');
    myGLCD.print("azul:  " , LEFT, 40); myGLCD.printNumI(azul  , 50, 40, 3, ' ');

    pitidos(1);                           // un pitido par  indicar la toma de una medida
    //digitalWrite(PIN_LED_SENSOR, true);   //apagar
  }
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        FUNCIONES DE ACCESO A LAS MEDICIONES DEL TCS3414cs
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//   FUNCION PARA OBTENER TODAS LAS COMPONENTES DEL COLOR
//    USANDO IGUALES PARAMETROS PARA CADA COMPONENTE      
//========================================================

struct SensorData_type  detectarColor()
/*
 * Esta funcion hace llamadas a colorRead(comando) lo que nos permite seleccionar 
 * los distintos canales del ADC mediante el acceso al registro COMMAND 
 * Descripcion del registro COMMAND para acceso a los distintos canales:
  - canal green 0xB0  --> B101 10000   --> direccion 10h
  - canal red   0xB2  --> B101 10010   --> direccion 12h
  - canal blue  0xB4  --> B101 10100   --> direccion 14h
  - canal white 0xB6  --> B101 10110   --> direccion 16h 
 * CONSULTAR EL DATASHEET DEL SENSOR PARA MAS INFORMACION
 */
{
  struct SensorData_type  w_rgb;          // dato del tipo  'struct SensorData_type'

  unsigned int green  = colorRead(0xB0);  
  unsigned int red    = colorRead(0xB2);
  unsigned int blue   = colorRead(0xB4);
  unsigned int white  = colorRead(0xB6);

  w_rgb.w = white;
  w_rgb.r = red;
  w_rgb.g = green;
  w_rgb.b = blue;

  return w_rgb;
}



//========================================================
//   FUNCION PARA OBTENER TODAS LAS COMPONENTES DEL COLOR 
//     USANDO DISTINTOS PARAMETROS PARA CADA COMPONENTE     
//========================================================

struct SensorData_type  detectarSoloComponentes()
/*
 * Esta funcion hace llamadas a colorRead(comando) lo que nos permite seleccionar 
 * los distintos canales del ADC mediante el acceso al registro COMMAND
 * Previamente se modifican los parametros de acceso, para compensar 
 * las variacioens de sensibilidad entre canales (Pruebas empiricas)
 
 * Descripcion del registro COMMAND para acceso a los distintos canales:
  - canal green 0xB0  --> B101 10000   --> direccion 10h
  - canal red   0xB2  --> B101 10010   --> direccion 12h
  - canal blue  0xB4  --> B101 10100   --> direccion 14h
  - canal white 0xB6  --> B101 10110   --> direccion 16h 
 * CONSULTAR EL DATASHEET DEL SENSOR PARA MAS INFORMACION
 */
{
  struct SensorData_type  w_rgb;            // dato del tipo  'struct SensorData_type'
                                            // byte gain(1,4,16,64), byte prescaler(1,2,4,8,16,32,64)
  TSC3414_set_gain_and_prescaler (16, 1);   //16,1 comun a los tres canales

  //rojo
  //TSC3414_set_gain_and_prescaler (16, 1);  
  TSC3414_set_Manual_Timing (270);
  unsigned int red    = colorRead(0xB2);
  unsigned int white  = colorRead(0xB6);    //la leemos en una de las tres medciones...por ejemplo en la roja
  
  //verde
  //TSC3414_set_gain_and_prescaler (16, 1);  
  TSC3414_set_Manual_Timing (120);  //247
  unsigned int green  = colorRead(0xB0);

  //azul
  //TSC3414_set_gain_and_prescaler (64, 4);  
  TSC3414_set_Manual_Timing (303);   //343
  unsigned int blue   = colorRead(0xB4);

  w_rgb.w = white;
  w_rgb.r = int((float(red)* 0.9));     //red,     cambiar el 0.9 si se desea hacer pequeñas compensaciones
  w_rgb.g = int((float(green)* 0.9));   //green;   al margen de ganacia y preescalar
  w_rgb.b = int((float(blue)* 0.9));    //blue;    Poniendo 1.0 se dejan los valores tal como se leen

  return w_rgb;
}




//========================================================
//  FUNCION PARA ACCESO A UN REGISTRO DE COLOR DEL SENSOR     
//========================================================

unsigned int colorRead(int canalColor)
/*
 * accedemos al registro COMMAND para seleccionar la direccion de cada canal de color del ADC
 * green 0xB0  
 * red   0xB2
 * blue  0xB4
 * white 0xB6 
 * CONSULTAR EL DATASHEET DEL SENSOR PARA MAS INFORMACION
 */
{
  unsigned int byte_L;
  unsigned int byte_H;
  unsigned int dato_Completo;
  
  Wire.beginTransmission(SENSOR_COLOR_ADDR);
  Wire.write(canalColor);
  Wire.endTransmission();
  
  Wire.beginTransmission(SENSOR_COLOR_ADDR);
  Wire.requestFrom(SENSOR_COLOR_ADDR,2);
  //while(Wire.available()==0);
  
  byte_L = Wire.read();
  byte_H = Wire.read();
  
  Wire.endTransmission();
  
  dato_Completo = (byte_H << 8) + byte_L;

  return dato_Completo;
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//       SECCION PARA MODIFICACION DE REGISTROS DE CONFIGURACION DEL TCS3414TC
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//    FUNCION PARA INICIAR EL SENSOR Y LOS CANALES ADC     
//========================================================

/* ---  Control Register (00h) --- */
void TSC3414_begin()
/*
 * accedemos al registro 00h y activamos el sensor y los canales ADC
 * CONSULTAR EL DATASHEET DEL SENSOR PARA MAS INFORMACION
 */
{
  Wire.beginTransmission(SENSOR_COLOR_ADDR);    //seleccionar al sensor
  Wire.write(0x80);                             //Command Register BIN 1       00              00000
                                                //                     select  protocolo byte  direccion
  
  Wire.write(0x03);                             //habilitar canales ADC
  Wire.endTransmission();                       //finalizar transmision
}



//========================================================
//    FUNCION PARA MODIFICAR EL TIEMPO DE SENSADO    
//========================================================

/* ---  Mediante Timing Register (01h) --- */
void TSC3414_set_Timing_Register (int milisegundos)
/*
 * accedemos al registro 01h y modificamos los tiempos de sensado
 * CONSULTAR EL DATASHEET DEL SENSOR PARA MAS INFORMACION
 */
{                                                  
  byte Timing_Register = B00000000;             //por defecto 12 ms

  if (milisegundos == 100){
    Timing_Register = B00000001;
  }
  
  if (milisegundos == 400){
    Timing_Register = B00000010;
  }

  Wire.beginTransmission(SENSOR_COLOR_ADDR);    //seleccionar al sensor
  Wire.write(0x81);                             //Timing Register 01h                             
                                                //Command Register BIN 1       00              00001
                                                //                     select  protocolo byte  direccion

  Wire.write(Timing_Register);                  //escribir el el valor de tiempo de integracion en Timing Register
  Wire.endTransmission();                       //finalizar transmision

}


//========================================================
//     CONTROL MANUAL DEL LOS TIEMPOs DE SENSADO   
//========================================================

/* ---  Control manual de los tiempos de sensado --- */
void TSC3414_set_Manual_Timing (int milisegundos)
/*
 * control manual de los tiempos de sensado
 * ¡OJO! al usar este modo queda desactivado el ADC. 
 * Sera necesario activarlo nuevamente si se desea hacer lecturas automaticas
 * CONSULTAR EL DATASHEET DEL SENSOR PARA MAS INFORMACION
 */
{
  Wire.beginTransmission(SENSOR_COLOR_ADDR);    //seleccionar al sensor
  Wire.write(0x80);                             //Command Register (select  protocolo byte) 
  Wire.write(0x01);                             //Deshabilitar el ADC
  Wire.endTransmission();

  Wire.beginTransmission(SENSOR_COLOR_ADDR); 
  Wire.write(0x81);                             //Timing Register (select  protocolo byte)
  Wire.write(0x10);                             //activar integracion manual
  Wire.endTransmission();
  
  Wire.beginTransmission(SENSOR_COLOR_ADDR); 
  Wire.write(0x80);                             //Command Register (select  protocolo byte)
  Wire.write(0x03);                             //habilitar canales ADC
  Wire.endTransmission();
  
  delay(milisegundos);                          //pausa igual al tiempo de sensado/integracion deseado
  
  Wire.beginTransmission(SENSOR_COLOR_ADDR); 
  Wire.write(0x80);                             //Command Register (select  protocolo byte)
  Wire.write(0x01);                             //Deshabilitar el ADC
  Wire.endTransmission();                       //finalizar transmision
}


/* ---  Interrupt control Register (02h) --- */ //no programado aun
/* ---  Interrupt Source Register (03h) --- */  //no programado aun
/* ---  ID Register (04h) --- */                //no programado aun


//========================================================
//   FUNCION PARA MODIFICAR LA GANANCIA Y EL 'PRESCALER'   
//========================================================
/* ---  Gain Register (07h) --- */
void TSC3414_set_gain_and_prescaler (byte gain, byte prescaler)
/*
 * accedemos al registro 07h y modificamos la ganancia y el preescalado de las mediciones
 * CONSULTAR EL DATASHEET DEL SENSOR PARA MAS INFORMACION
 */
{  
  byte gain_Register = B00000000;         //por defecto ganacia 1x
  if (gain == 4){
    gain_Register = B00010000;
  }
  if (gain == 16){
    gain_Register = B00100000;
  }
  if (gain == 64){
    gain_Register = B00110000;
  }  

  byte prescaler_Register = B00000000;    //por defecto preescalado 1
  
  if (prescaler == 2){
    prescaler_Register = B00000010;
  } 
  if (prescaler == 4){
    prescaler_Register = B00000011;
  }
  if (prescaler == 8){
    prescaler_Register = B00000100;
  }
  if (prescaler == 16){
    prescaler_Register = B00000101;
  } 
  if (prescaler == 32){
    prescaler_Register = B00000110;
  }
  if (prescaler == 64){
    prescaler_Register = B00000111;
  }

  byte gain_and_prescaler = gain_Register + prescaler_Register;
    
  Wire.beginTransmission(SENSOR_COLOR_ADDR);    //seleccionar al sensor
  Wire.write(0x87);                             //registro ganancia 07h
                                                //Command Register BIN 1       00              00111
                                                //                     select  protocolo byte  direccion
                                                
  Wire.write(gain_and_prescaler);               //escribimos el byte que contiene ganacia y preescalado
  Wire.endTransmission();
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    TECLADO / PULSADORES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  FUNCION PARA CONTROLAR EL ESTADO DEL PULSADOR
//========================================================

byte leer_Pulsador_Tactil()
/*
 * Funcion para atender la pulsacion de la tecla OK/seleccion
 */
{
  boolean FLAG_pulsacion = false;
  unsigned long momento_pulsar_boton = 0;
  unsigned long momento_soltar_boton = 0;
  byte tipo_pulsacion = 0;   // no ha habido pulsacion 
    
  boolean pulsacion = digitalRead(PIN_PULSADOR_SELECCION);  // leemos el estado del pulsador tactil
  if (pulsacion==true){   
    pitidos(1);  //1 pitido
    momento_pulsar_boton = millis();                        //'anotamos' el momento de la pulsacion
    delay(25);                                              //pausa para evitar rebotes
    while(pulsacion==true){
      pulsacion = digitalRead(PIN_PULSADOR_SELECCION);      // leemos el estado del pulsador tactil
    }
    momento_soltar_boton = millis();
    FLAG_pulsacion = true; 
  }
  
  unsigned long duracion_pulsacion = momento_soltar_boton - momento_pulsar_boton;
  //determinar la duracion de pulsacion para saber si es normal o larga

  if (FLAG_pulsacion==true){  //pulsacion corta
    if (duracion_pulsacion >= 450){  
      tipo_pulsacion = 2; //pulsacion larga
    }
    else{
      tipo_pulsacion = 1; //pulsacion normal
    }
  }
  return tipo_pulsacion;
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//      MODO PROGRAMAR PARAMETROS  (GESTION DEL MENU)
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
// ASIGNAR VALORES AL MODO DE MEDICION
//========================================================
void programar_Parametros()
/*
 * Generacion del menu para entrada de parametros que son variables GLOBALES
  - tiempo_integracion_sensor
  - ganancia_sensor
  - prescaler_sensor
 * con cada pulsacion aceptamos el valor mostrado en pantalla
 * 'saltando' automaticamente al siguente campo.
 * (Podemos corregir un valor ya aceptado realizando una pulsacion larga)
 */
{
  myGLCD.clrScr();
  
  byte pulsacion = 0;
  int REG_ESTADO = 1;                   // variable que indicara en que campo del menu de seleccion estamos
  boolean FLAG_selec_mode = true;       // bandera para indicar que aun no hemos terminado de introducir el tiempo                               

  int valor_mapeado;                    // variable para el valor del potenciometro ajsutado al tipo de campo
                                        //sobre el que estamos trabajando     
  byte lista_ganacia[] = {1,4,16,64};
  byte lista_prescaler[] = {1,2,4,8,16,32,64};
  FLAG_color_compensado = false;
  
  while (FLAG_selec_mode == true){
    int valorPotenciometro = analogRead(POTENCIOMETRO);   // el potenciometro nos permitira ingresar valores 
                                                          // del rango -1 y 59. 
                                                          // aceptar el valor ('--') le indica a la rutina que deseamos 
                                                          // volver sobre el valor anterior para corregirlo
    if(REG_ESTADO < 0){
      FLAG_color_compensado = true;
      pitidos(3);
      return;
      REG_ESTADO = 1;
    }
    
    if (REG_ESTADO == 1){
      myGLCD.print("> TIME" , LEFT, 0);
      valor_mapeado = map(valorPotenciometro,0,1023, 11,1510);
      if (valor_mapeado < 12){
        valor_mapeado=12;
      }     
  
      if (valor_mapeado > 1500){
        valor_mapeado = 1500;
      }
      tiempo_integracion_sensor = valor_mapeado;
      tiempo_integracion_sensor = tiempo_integracion_sensor / 10;  // para obligar a mostar de 10 en 10
      tiempo_integracion_sensor = tiempo_integracion_sensor * 10;  // para obligar a mostar de 10 en 10
      myGLCD.printNumI(tiempo_integracion_sensor, 54, 0, 4, ' ');
    }
    
    if (REG_ESTADO == 2){
      myGLCD.print("> GAIN" , LEFT, 8);
      valor_mapeado = map(valorPotenciometro,5,1020, -1,4);
      if (valor_mapeado < 0){
        valor_mapeado=0;
      }     
  
      if (valor_mapeado > 3){
        valor_mapeado=3;
      }
      ganancia_sensor = lista_ganacia[valor_mapeado];
      myGLCD.printNumI(ganancia_sensor, 64, 8, 2, ' ');
    }
     
    if(REG_ESTADO == 3){
      myGLCD.print("> PRESCAL" , LEFT, 16);
      
      valor_mapeado = map(valorPotenciometro,5,1020, -1,7);
      if (valor_mapeado < 0){
        valor_mapeado=0;
      }     
  
      if (valor_mapeado > 6){
        valor_mapeado=6;
      }
      prescaler_sensor = lista_prescaler[valor_mapeado]; 
      myGLCD.printNumI(prescaler_sensor, 64, 16, 2, ' ');     
    }       

    if (REG_ESTADO == 4){
      //si se completn la programacion de parametros mostramos un mensaje y esperamos una nueva pulsacion
      myGLCD.print("Pulsa OK para" , CENTER, 32);
      myGLCD.print("aplicar ajuste" , CENTER, 40);
    }

            
    if (REG_ESTADO==5){                 // completado el proceso de introducion del tiempo a temporizar
      FLAG_selec_mode = false;      // se desactiva la posibilidad de volver atras para modificar el tiempo
      myGLCD.clrScr();
    } 
        
    pulsacion = leer_Pulsador_Tactil(); // leemos el estado del pulsador tactil

    if (pulsacion == 1){
      REG_ESTADO++;       // si las condiciones son correctas se acepta el valor para ese campo y se salta al siguiente
      pulsacion = 0;      //anulamos el estado de la pulsacion por precaucion
    }
    if (pulsacion == 2){
      REG_ESTADO--;       // si hay pulsacion larga se vuelve al campo anterior
      pulsacion = 0;      //anulamos el estado de la pulsacion por precaucion                    
    }
  }
  //fin del menu de seleccion de opciones

  return;
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//      SONIDO
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  FUNCION para generar PITIDOS
//========================================================

void pitidos(byte numero_pitidos)
{
  for (byte i=0; i<numero_pitidos; i++){
    tone(PIN_ZUMBADOR, FRECUENCIA);
    delay (TIEMPO_SONIDO);
    noTone(PIN_ZUMBADOR);
    if(numero_pitidos > 1){
      delay (TIEMPO_SILENCIO);
    }
  }
}


//*******************************************************
//                    FIN DE PROGRAMA
//*******************************************************

