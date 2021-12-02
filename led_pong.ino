#define EB_DEB 100
#include <EncButton.h>

#define PLAYER_1_COLOR CRGB::Blue
#define PLAYER_2_COLOR CRGB::Red

#define BALL_COLOR CRGB::Yellow
#define BALL_SIZE 1
#define BALL_SPEED_MIN 40
#define BALL_SPEED_MAX 90

#define LED_PIN 13
#define NUM_LEDS 119
#define LED_ZOOM 100

#define GATE_SIZE 15
#define GAME_SPEED_DEFAULT 100
#define GAME_MAP_SIZE (NUM_LEDS * LED_ZOOM)

#include <FastLED.h>
CRGB leds[NUM_LEDS];
unsigned long int last_updated_at = 0;
unsigned int current = 0;

#define LED_START 0
#define LED_END NUM_LEDS-1
#define LED_CENTER NUM_LEDS/2
#define DIR_RIGHT 0
#define DIR_LEFT 1

bool is_game_running = false;

unsigned long int gameTimer = 0;
int ballPosition = LED_CENTER;
int ballSpeed = 5;

bool isPongMode = true;

EncButton<EB_TICK, 2> btn1(INPUT_PULLUP);
EncButton<EB_TICK, 3> btn2(INPUT_PULLUP);



void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);

  FastLED.addLeds<WS2812, LED_PIN, RGB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.setBrightness(200);

  if (digitalRead(2) == LOW) {
    isPongMode = false;
  }

  if (isPongMode) {
    startGame();
  }
  
}

void drawGates() {
  for (byte i = 0; i < GATE_SIZE; i++) {
    leds[LED_START + i] = CHSV(160, 128, 100);
    leds[LED_END - i] = CHSV(160, 128, 100);
  }

  leds[NUM_LEDS / 2] = CRGB::White;
  leds[NUM_LEDS / 2 + 1] = CRGB::White;
}

void startGame() {
  Serial.println("----------------------- NEW GAME -------------------------");
  FastLED.clear();
  randomSeed(analogRead(0));
  ballSpeed = random(0, 100) % 2 == 0 ? BALL_SPEED_MIN : -BALL_SPEED_MIN;
  ballPosition = (NUM_LEDS * LED_ZOOM) / 2;

  Serial.print("Init ball speed: ");
  Serial.println(ballSpeed);

  Serial.print("Init ball position: ");
  Serial.println(ballPosition);

  for (byte i = 0; i < 5; i++) {
    leds[LED_CENTER + i] = CRGB::Red;
    leds[LED_CENTER - i] = CRGB::Red;
  }
  FastLED.show();

  delay(1000);

  for (byte i = 0; i < 5; i++) {
    leds[LED_CENTER + i] = CRGB::Yellow;
    leds[LED_CENTER - i] = CRGB::Yellow;
  }
  FastLED.show();

  delay(1000);

  for (byte i = 0; i < 5; i++) {
    leds[LED_CENTER + i] = CRGB::Green;
    leds[LED_CENTER - i] = CRGB::Green;
  }
  FastLED.show();

  delay(2000);

  FastLED.clear();

  drawGates();

  FastLED.show();

  is_game_running = true;
}

void gameOver(byte playerIndex) {
  is_game_running = false;
  for (byte i = LED_START; i <= LED_END; i++) {
    if (playerIndex == 1) {
      leds[LED_END - i] = PLAYER_2_COLOR;

    } else {
      leds[i] = PLAYER_1_COLOR;
    }

    delay(20);
    FastLED.show();
  }

  delay(2000);

  startGame();
}

int getAcceleratedSpeed(int ballPosition) {
  int ballPositionInGates = 0;

  if (ballPosition >= 0 && ballPosition < NUM_LEDS * LED_ZOOM / 2) {
    ballPositionInGates = ballPosition;
  }

  if (ballPosition >= NUM_LEDS * LED_ZOOM / 2 + (1 * LED_ZOOM) && ballPosition < NUM_LEDS * LED_ZOOM) {
    ballPositionInGates = NUM_LEDS * LED_ZOOM - ballPosition - (1 * LED_ZOOM);
  }

  Serial.print("Ball position in gates: ");
  Serial.println(ballPositionInGates);

  return map(ballPositionInGates, 0, GATE_SIZE * LED_ZOOM - (1 * LED_ZOOM), BALL_SPEED_MAX, BALL_SPEED_MIN);
}

void drawBall(int pos) {
  leds[(pos / LED_ZOOM)] = BALL_COLOR;
}

void gameRoutine() {
  if (! is_game_running) {
    return;
  }

  if (btn1.click()) {
    if (ballPosition > GATE_SIZE * LED_ZOOM) {
      gameOver(1);
      return;
    }

    if (ballPosition >= LED_START && ballPosition < (NUM_LEDS * LED_ZOOM / 2)) {
      ballSpeed = abs(ballSpeed);
      ballSpeed = getAcceleratedSpeed(ballPosition);

      Serial.print("Ball Position: ");
      Serial.println(ballPosition);

      Serial.print("New Speed: ");
      Serial.println(ballSpeed);
    }
  }

  if (btn2.click()) {
    if (ballPosition < (NUM_LEDS - GATE_SIZE) * LED_ZOOM) {
      gameOver(1);
      return;
    }

    if (ballPosition > (NUM_LEDS * LED_ZOOM / 2 + 1) && ballPosition < NUM_LEDS * LED_ZOOM) {
      ballSpeed = getAcceleratedSpeed(ballPosition);
      ballSpeed = abs(ballSpeed) * -1;

      Serial.print("Ball Position: ");
      Serial.println(ballPosition);

      Serial.print("New Speed: ");
      Serial.println(ballSpeed);
    }
  }

  if (millis() - gameTimer >= 10) {
    gameTimer = millis();

    ballPosition += ballSpeed;

    if (ballPosition <= LED_START) {
      ballPosition = LED_START;
      ballSpeed *= -1;
      Serial.print("Game over. Ball Position: ");
      Serial.println(ballPosition);
      gameOver(1);
      return;
    }
    //
    //
    if (ballPosition  >= (NUM_LEDS * LED_ZOOM)) {
      ballPosition = NUM_LEDS * LED_ZOOM;
      ballSpeed *= -1;
      Serial.print("Game over. Ball Position: ");
      Serial.println(ballPosition);
      gameOver(2);
      return;
    }


    //
    FastLED.clear();
    //
    drawGates();

    drawBall(ballPosition);
    //
    FastLED.show();
  }
}

int tar, cur;      // храним позицию и цель
byte color;        // храним цвет
void catGameLoop() {
  if (tar == cur) {             // позиция совпала
    tar = random(0, NUM_LEDS);   // новая случайная позиция
    delay(random(500, 2000));                 // ждём
  }
  cur += (cur < tar) ? 1 : -1;  // направление движения
  // снижаем яркость всей ленты
  for (int i = 0; i < NUM_LEDS; i++) leds[i].fadeLightBy(180);
  leds[cur].setHue(color);  // зажигаем светодиод
  color++;                  // меняем цвет
  FastLED.show();           // выводим
  delay(random(5, 15));                 // ждём
}

void loop() {
  if (isPongMode) {
    btn1.tick();
    btn2.tick();
    gameRoutine();
  } else {
    catGameLoop();
  }

}
