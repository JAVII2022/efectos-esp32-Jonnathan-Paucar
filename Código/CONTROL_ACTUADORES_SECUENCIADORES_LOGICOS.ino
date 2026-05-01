// LED para efecto breathing (PWM)
#define LED_BREATHING 4      

// LEDs para la ruleta (Coche Fantástico)
#define LED_VERDE 13      
#define LED_AMARILLO 12
#define LED_ROJO 14
#define LED_BLANCO 27
#define LED_AZUL 26

// Botón para cambiar de estado
#define BOTON 15              

// Variables para el breathing
volatile int pwmValue = 0;
int step = 5;
int brightness = 0;

// Variables para la ruleta
volatile int ledActual = 0;
volatile unsigned long lastStep = 0;
const unsigned long STEP_TIME = 500;  

// Máquina de estados
volatile int state = 1;        
volatile int contador = 0;     
volatile int botonState = 0;   

// Timer hardware
hw_timer_t * timer = NULL;

// Array con los pines de los LEDs de la ruleta
int ruletaPins[] = {LED_VERDE, LED_AMARILLO, LED_ROJO, LED_BLANCO, LED_AZUL};

// Prototipos
void IRAM_ATTR onTimer();
void cambiarEstado();
void ejecutarBreathing();
void ejecutarRuleta();

void setup() 
{
  Serial.begin(9600);
  Serial.println("Sistema Control de Actuadores");
  
  // Configurar pines de salida
  pinMode(LED_BREATHING, OUTPUT);
  for(int i = 0; i < 5; i++) 
  {
    pinMode(ruletaPins[i], OUTPUT);
    digitalWrite(ruletaPins[i], LOW);
  }
  
  // Configurar botón con pull-up
  pinMode(BOTON, INPUT_PULLUP);
  
  // Configurar timer para ESP32 (API para versiones muy antiguas)
  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000, true, 0);  
  
  Serial.println("Estado 1: Modo Respiración");
}

void loop() 
{
  // Leer botón y cambiar estado si es necesario
  cambiarEstado();
  
  // Ejecutar el estado actual
  if (state == 1)
  {
    ejecutarBreathing();
  } 
  else if (state == 2)
  {
    ejecutarRuleta();
  }
}

// ISR del Timer (1ms)
void IRAM_ATTR onTimer() 
{
  contador++;  // Incrementar contador cada 1ms
}
// Función para cambiar de estado con debounce
void cambiarEstado()
{
  static int lastButtonState = HIGH;
  static int debounceCounter = 0;
  const int DEBOUNCE_TIME = 50;  
  
  int reading = digitalRead(BOTON);
  
  if(reading != lastButtonState) 
  {
    debounceCounter = contador;
  }
  
  if((contador - debounceCounter) > DEBOUNCE_TIME) 
  {
    if(reading == LOW && botonState == 0) 
    {  
      botonState = 1;
      
      // Cambiar de estado
      if(state == 1) 
      {
        state = 2;
        // Reiniciar ruleta
        ledActual = 0;
        lastStep = contador;
        for(int i = 0; i < 5; i++) 
        {
          digitalWrite(ruletaPins[i], LOW);
        }
        analogWrite(LED_BREATHING, 0);  // Apagar LED breathing
        Serial.println("Estado 2: Modo Ruleta Rusa ");
        Serial.println("LED 1 encendido");
        digitalWrite(ruletaPins[0], HIGH);  // Encender primer LED
      } 
      else 
      {
        state = 1;
        // Reiniciar breathing
        pwmValue = 0;
        step = 5;
        brightness = 0;
        for(int i = 0; i < 5; i++) 
        {
          digitalWrite(ruletaPins[i], LOW);
        }
        Serial.println("Estado 1: Modo Respiración");
      }
    } else if(reading == HIGH) 
    {
      botonState = 0;
    }
  }
  lastButtonState = reading;
}

// Función para generar PWM manualmente
void generarPWM(int pin, int valor) 
{
  // PWM manual
  static unsigned long lastPWMTime = 0;
  static int currentValue = 0;
  static bool pwmState = LOW;
  
  unsigned long currentMicros = contador * 1000;  
  
  int periodo = 2000; 
  int tiempoOn = (valor * periodo) / 255;
  
  if(pwmState == HIGH && (currentMicros - lastPWMTime) >= tiempoOn) 
  {
    digitalWrite(pin, LOW);
    pwmState = LOW;
    lastPWMTime = currentMicros;
  } else if(pwmState == LOW && (currentMicros - lastPWMTime) >= (periodo - tiempoOn)) 
  {
    if(valor > 0) 
    {
      digitalWrite(pin, HIGH);
      pwmState = HIGH;
    }
    lastPWMTime = currentMicros;
  }
}
// Estado 1: Efecto Breathing (respiración PWM)
void ejecutarBreathing()
{
  static unsigned long lastUpdate = 0;
  const unsigned long UPDATE_INTERVAL = 15;  
  
  if((contador - lastUpdate) >= UPDATE_INTERVAL) 
  {
    lastUpdate = contador;
    
    brightness += step;
    
    if(brightness >= 255) 
    {
      brightness = 255;
      step = -5;
    } 
    else if(brightness <= 0) 
    {
      brightness = 0;
      step = 5;
    }
    analogWrite(LED_BREATHING, brightness);
    
    // Imprimir valor actual en monitor serial
    Serial.print("PWM Value: ");
    Serial.println(brightness);
  }
}

// Estado 2: Ruleta Rusa (Coche Fantástico)
void ejecutarRuleta()
{
  if((contador - lastStep) >= STEP_TIME) 
  {
    lastStep = contador;
    // Apagar LED actual
    digitalWrite(ruletaPins[ledActual], LOW);

    // Avanzar al siguiente LED
    ledActual++;
    
    // Reiniciar al llegar al final
    if(ledActual >= 5) 
    {
      ledActual = 0;
    }
    
    // Encender el nuevo LED
    digitalWrite(ruletaPins[ledActual], HIGH);
    
    // Imprimir estado en monitor serial
    Serial.print("LED ");
    Serial.print(ledActual + 1);
    Serial.println(" encendido");
  }
}