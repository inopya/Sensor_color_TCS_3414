# Sensor_color_TCS_3414

 Lector de COLORES  con  TCS 3414TC
 
 Autor: Eulogio López Cayuela
 
 Versión 3.0     Fecha: 18/10/2017 
                                         


  NOTA SOBRE VERSIONES PREVIAS (no disponibles):
  
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
