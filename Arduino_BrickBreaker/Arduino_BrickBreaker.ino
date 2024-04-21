#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SEVEN_1_A 22
#define SEVEN_1_B 24
#define SEVEN_1_C 26
#define SEVEN_1_D 28
#define SEVEN_1_E 30
#define SEVEN_1_F 32
#define SEVEN_1_G 34

#define SEVEN_2_A 36
#define SEVEN_2_B 38
#define SEVEN_2_C 40
#define SEVEN_2_D 42
#define SEVEN_2_E 44
#define SEVEN_2_F 46
#define SEVEN_2_G 48

#define LOW_HEALTH_LED 6
#define MID_HEALTH_LED 7
#define HIGH_HEALTH_LED 8

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define GAP 1
#define MAX_BRICK_AMOUNT 30

#define JOY_X A0 
#define JOY_Y A1
#define JOY_BUTTON 2

#define UP_BUTTON 3
#define DOWN_BUTTON 4
#define SELECTION_BUTTON 5

#define A 8

#define BRICK_WIDTH 16
#define BRICK_HEIGHT 4

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT);

bool bIsGameStart;
bool bIsGameOver;
bool bIsGameQuit;
bool bIsGameWin;
bool bIsReborn;
bool bShouldHealthFallDown;
uint8_t brickAmount;

int16_t restartTextX = OLED_WIDTH;
int16_t quitTextX = OLED_WIDTH;

// Cursor positions in opening screen to choose option
uint8_t selectionCursorY;
uint8_t selectionCursorX;

uint8_t SELECTION_BUTTONPrev;
uint8_t UP_BUTTONPrev; // prevent running in each frame when button is staying pressed
uint8_t DOWN_BUTTONPrev; // prevent running in each frame when button is staying pressed
uint8_t DOWN_BUTTONPrev2;

uint8_t openingScreenOptionIndex = 0;
uint8_t openingScreenOptionsY[3] = {25, 35, 45};
uint8_t openingScreenOptionsX[3] = {45, 45, 70};

int8_t nextChapterTimer;
uint8_t chapterNumber;

String modeText = "DARK";

typedef struct
{
  uint8_t maxHealth = 3;
  int8_t currentHealth = 3;
  uint8_t height = 3;
  uint8_t width = 20;
  float x;
  float y;
  float speed = 2;
  uint8_t score = 0;

  void paletteMove()
  { 
    //int xValue = analogRead(JOY_X);
    int yValue = analogRead(JOY_Y);
  
    if (yValue < 10 && x < OLED_WIDTH - width) 
    {
      x += speed; 
    }
      
    if (yValue > 1000 && x > 0) 
    {
      x -= speed; 
    }
  }  
} Palette;

typedef struct 
{
  uint8_t x;
  uint8_t y;
  uint8_t bottom;
  bool shouldDown = false;
  
  void fallDown()
  {
    if(shouldDown)
    {
      y++;
      bottom++;
    }
  }

  void collisionCheck(Palette& _palette, void (*_updateHealthLeds)())
  {
    if (_palette.x < x && _palette.x + _palette.width > x && _palette.y <= bottom)
    {
      if (_palette.currentHealth < 3)
      {
        _palette.currentHealth++;
        _updateHealthLeds();

      }
      shouldDown = false;
    }
    else if (y > OLED_HEIGHT)
    {
      shouldDown = false;
    }
  }
} Heart;

typedef struct 
{
  uint8_t x;
  uint8_t y;
  bool isHit;
} Brick;

typedef struct
{
  float radius = 2;
  float x;
  float y;
  float directionX; // - -> left, + -> rigth
  float directionY; // -1 -> up, 1 -> down
  bool shouldMove = false;

  void ballMove(Palette& _palette)
  {
    if (shouldMove)
    {
      y += directionY;
      x += directionX;
    }

    else
    {
      x = _palette.x + _palette.width / 2;
    }

    if (digitalRead(JOY_BUTTON) == 0)
    {
      shouldMove = true;
    }
  }

  void collisionChecks(Palette& _palette, void (*_updateHealthLeds)(), void (*_updateScoreBoard)(), bool (*_shouldHeartDown)(), Heart _hearts[], Brick _bricks[])
  {    
    // LEFT AND RIGHT SCREEN BORDER COLLISION CHECK
    if(x < radius || x > OLED_WIDTH - radius)
    {
      directionX *= -1;  
    }

    // UP SCREEN BORDER COLLISION CHECK
    if(y < radius)
    {
      directionY *= -1;
    }
    // DOWN SCREEN BORDER COLLISION CHECK
    if(y > OLED_HEIGHT - radius)
    {
      _palette.currentHealth--;
      _updateHealthLeds();
      if(_palette.currentHealth <= 0)
      {
        bIsGameOver = true;
        delay(3000);       
        return;
      }
      bIsReborn = true;     
    }
    
    // PALETTE Y DIRECTION COLISION CHECK
    if(x <= _palette.x + _palette.width && x >= _palette.x && y + radius >= _palette.y)
    {
      directionY *= -1;
    }

    // BRICKS COLLISION CHECK
    for(int i = 0; i < MAX_BRICK_AMOUNT; i++)
    {
      // UP AND DOWN BORDER CHECK
      if( _bricks[i].isHit == false && 
          x <= _bricks[i].x + BRICK_WIDTH && // LEFT BORDER 
          x >= _bricks[i].x && // RIGTH BORDER
          ((y > _bricks[i].y && y - _bricks[i].y <= radius + BRICK_HEIGHT) || // DOWN BORDER
           (y < _bricks[i].y && y - _bricks[i].y >= -radius)) // UP BORDER
        )
      {
        _bricks[i].isHit = true;
        directionY *= -1;
        _palette.score++;
        _updateScoreBoard();
        _hearts[i].shouldDown = _shouldHeartDown();
      }


      else if( _bricks[i].isHit == false && 
          y <= _bricks[i].y + BRICK_HEIGHT &&
          y >= _bricks[i].y &&
          ((x > _bricks[i].x && x - _bricks[i].x <= radius + BRICK_WIDTH) || 
           (x < _bricks[i].x && x - _bricks[i].x >= -radius))
        )
      {
        _bricks[i].isHit = true;
        directionX *= -1;
        _palette.score++; 
        _updateScoreBoard();
        _hearts[i].shouldDown = _shouldHeartDown();
      }

      if(_palette.score == brickAmount)
      {
        bIsGameWin = true;
      }
    }
  }

} Ball;

Ball ball;
Palette palette;

Brick bricks[MAX_BRICK_AMOUNT];
Heart hearts[MAX_BRICK_AMOUNT];

enum OpeningOptions
{
  START,
  QUIT,
  MODE 
};

enum Mode
{
  DARK, 
  LIGHT
};

OpeningOptions currentOption;
Mode currentMode;

void initBegins()
{
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void initVariables()
{   
    pinMode(UP_BUTTON, INPUT);
    pinMode(DOWN_BUTTON, INPUT);
    pinMode(SELECTION_BUTTON, INPUT);

    pinMode(SEVEN_1_A, OUTPUT);
    pinMode(SEVEN_1_B, OUTPUT);
    pinMode(SEVEN_1_C, OUTPUT);
    pinMode(SEVEN_1_D, OUTPUT);
    pinMode(SEVEN_1_E, OUTPUT);
    pinMode(SEVEN_1_F, OUTPUT);
    pinMode(SEVEN_1_G, OUTPUT);

    pinMode(SEVEN_2_A, OUTPUT);
    pinMode(SEVEN_2_B, OUTPUT);
    pinMode(SEVEN_2_C, OUTPUT);
    pinMode(SEVEN_2_D, OUTPUT);
    pinMode(SEVEN_2_E, OUTPUT);
    pinMode(SEVEN_2_F, OUTPUT);
    pinMode(SEVEN_2_G, OUTPUT);

    pinMode(LOW_HEALTH_LED, OUTPUT);
    pinMode(MID_HEALTH_LED, OUTPUT);
    pinMode(HIGH_HEALTH_LED, OUTPUT);

    bIsGameStart = false;
    bIsGameOver = false;
    bIsGameQuit = false;
    bIsReborn = false;
    bIsGameWin = false;
    bShouldHealthFallDown = false;
    
    selectionCursorX = 45;
    selectionCursorY = 25;

    palette.x = OLED_WIDTH / 2 - palette.width / 2;
    palette.y = OLED_HEIGHT - palette.height;

    //ball.radius = 1;
    ball.x = palette.x + palette.width / 2;
    ball.y = palette.y - 2 * ball.radius;
    float initdirectionX = random(0, 21);
    ball.directionX = initdirectionX / 10 - 1;
    ball.directionY = -1.5f; // -1 -> up, 1 -> down

    SELECTION_BUTTONPrev = LOW;
    UP_BUTTONPrev = LOW;
    DOWN_BUTTONPrev = LOW;

    currentOption = OpeningOptions::START;
    currentMode = Mode::DARK;
    brickAmount = 0;
    nextChapterTimer = 3;
    chapterNumber = 1;
}

void openingScreen()
{
  //Serial.println(openingScreenOptionIndex);
  // GAME NAME
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(25, 10);
  display.println("BRICK BREAKER");
  // START TEXT
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 25);
  display.println("START");
  // QUIT TEXT
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 35);
  display.println("QUIT");
  // MODE TEXT
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 45);
  display.println("MODE: ");
  // MODE TEXT
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(40, 45);
  display.println(modeText);
  // SELECTION CURSOR
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(selectionCursorX, selectionCursorY);
  display.println("<");

  // MOVING UP THROUGH THE OPTIONS 
  if(digitalRead(UP_BUTTON) == HIGH && UP_BUTTONPrev == LOW && openingScreenOptionIndex != 0)
  {
    openingScreenOptionIndex--;
    currentOption = (OpeningOptions)openingScreenOptionIndex;
    selectionCursorY = openingScreenOptionsY[openingScreenOptionIndex];
    selectionCursorX = openingScreenOptionsX[openingScreenOptionIndex];
  }
  UP_BUTTONPrev = digitalRead(UP_BUTTON);
  
  // MOVING DOWN THROUGH THE OPTIONS
  if(digitalRead(DOWN_BUTTON) == HIGH && DOWN_BUTTONPrev == LOW && openingScreenOptionIndex != 1)
  {
    if(openingScreenOptionIndex != 2)
    {
      openingScreenOptionIndex++;
      currentOption = (OpeningOptions)openingScreenOptionIndex;
      selectionCursorY = openingScreenOptionsY[openingScreenOptionIndex];
      selectionCursorX = openingScreenOptionsX[openingScreenOptionIndex];  
    }
  }

  else if(digitalRead(DOWN_BUTTON) == HIGH && DOWN_BUTTONPrev2 == LOW && openingScreenOptionIndex != 2)
  {
    openingScreenOptionIndex++;
    currentOption = (OpeningOptions)openingScreenOptionIndex;
    selectionCursorY = openingScreenOptionsY[openingScreenOptionIndex];
    selectionCursorX = openingScreenOptionsX[openingScreenOptionIndex];
  }

  DOWN_BUTTONPrev = digitalRead(DOWN_BUTTON);
  DOWN_BUTTONPrev2 = digitalRead(DOWN_BUTTON);
  
  // CHOOSE CURRENT OPTION
  if(digitalRead(SELECTION_BUTTON) == HIGH && SELECTION_BUTTONPrev == LOW)
  {
    switch(currentOption)
    {
      case OpeningOptions::START:
        bIsGameStart = true;
        break;
      case OpeningOptions::QUIT:
        bIsGameQuit = true;
      case OpeningOptions::MODE:
        if(currentMode == Mode::LIGHT)
        {
          currentMode = Mode::DARK;
          modeText = "DARK";
          display.invertDisplay(false);
        }  
        else if(currentMode == Mode::DARK)
        {
          currentMode = Mode::LIGHT;
          modeText = "LIGHT";
          display.invertDisplay(true);
        }  
        break;
    }
  }
  
  SELECTION_BUTTONPrev = digitalRead(SELECTION_BUTTON);
}

// Ekrandaki tuglalari olusturur
void initializeBricksPosition()
{
/*    for (uint16_t y = BRICK_HEIGHT + GAP; y < OLED_HEIGHT / 2 - BRICK_HEIGHT + GAP; y+= BRICK_HEIGHT + GAP)
    {
      for( uint16_t x = BRICK_WIDTH + GAP; x < OLED_WIDTH - BRICK_WIDTH+ GAP; x += BRICK_WIDTH + GAP)
      {
        bricks[brickAmount].x = x;
        bricks[brickAmount].y = y;
        hearts[brickAmount].x = x + (BRICK_WIDTH / 2); 
        hearts[brickAmount].y = y + (BRICK_HEIGHT / 2); 
        hearts[brickAmount].bottom = hearts[brickAmount].y + 5; 
        brickAmount++;        
      }
    }*/
}

void drawBricks()
{
  for (int i = 0; i < MAX_BRICK_AMOUNT; i++)
  {
     if(bricks[i].isHit == false)
     {
       display.fillRect(bricks[i].x, bricks[i].y, BRICK_WIDTH, BRICK_HEIGHT, WHITE); 
     }
  }
}

void drawPalette()
{
  display.fillRect(palette.x, palette.y, palette.width, palette.height, WHITE);
}

void drawBall()
{
  display.fillCircle(ball.x, ball.y, ball.radius, WHITE);
}

void drawHealth()
{
  for (int i = 0; i < MAX_BRICK_AMOUNT; i++)
  {
     if(hearts[i].shouldDown == true)
     {
           display.fillCircle(hearts[i].x - 2, hearts[i].y, 2, WHITE);
           display.fillCircle(hearts[i].x + 2, hearts[i].y, 2, WHITE);
           display.fillTriangle(hearts[i].x - 4, hearts[i].y, hearts[i].x + 4, hearts[i].y, hearts[i].x , hearts[i].y + 5, WHITE);
     }
  }
}

bool shouldHeartDown()
{
  return random(0, 101) > 90;
}

void gameOverScreen()
{
   // GAME OVER TEXT
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("GAME OVER");
  // SCORE TEXT
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(40, 35);
  display.print("SCORE : ");
  display.println(palette.score);
  // RESTART TEXT
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(restartTextX, 50);
  display.println("Press Joystick to Back Start");
  restartTextX--;
  if(restartTextX < -150)
  {
    restartTextX = OLED_WIDTH;
  }
}

void quitScreen()
{
  //
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(quitTextX, 20);
  display.println("Thank You For Your Interest In Our Game");
  //
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(quitTextX, 40);
  display.println("We'd like to see you again :)");
  
  quitTextX--;
  if(quitTextX < -175)
  {
    quitTextX = OLED_WIDTH;
  }
}

void reborn()
{
  ball.shouldMove = false;

  palette.x = OLED_WIDTH / 2 - palette.width / 2;
  palette.y = OLED_HEIGHT - palette.height;
  ball.x = palette.x + palette.width / 2;
  ball.y = palette.y - 2 * ball.radius;
  float initdirectionX = random(0, 21);
  ball.directionX = initdirectionX / 10 - 1;
  ball.directionY = -1.5f; // -1 -> up, 1 -> down
}

void backToMenu()
{
  restartTextX = OLED_WIDTH;
  bIsGameOver = false;
  palette.score = 0;
  palette.currentHealth = palette.maxHealth;
  updateScoreBoard();
  updateHealthLeds();


  initVariables();
  setLevel();

  for(int i = 0; i < MAX_BRICK_AMOUNT; i++)
  {
    bricks[i].isHit = false;
    hearts[i].x = bricks[i].x + (BRICK_WIDTH / 2);  
    hearts[i].y = bricks[i].y + (BRICK_HEIGHT / 2);
    hearts[i].bottom = hearts[i].y + 5;
  }
}

void winScreen()
{
  display.setTextSize(2);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(20, 10);
  display.println("YOU WON");
  
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(0, 35);
  display.println("Ready to Next Chapter");
  
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(65, 50);
  display.println(nextChapterTimer);
  delay(1000);
  nextChapterTimer--;

  if(nextChapterTimer < 0)
  {
      chapterNumber++;
      palette.score = 0;
      nextChapterTimer = 3;
      palette.currentHealth = 3;
      setLevel();
      updateBallSpeedForNextChapter(ball);
      reborn();
      bIsGameWin = false;
  }
}

void setLevel()
{
  brickAmount = 0;
  
  if (chapterNumber == 1)
  {
    for (uint16_t y = BRICK_HEIGHT + GAP; y < OLED_HEIGHT / 2 - BRICK_HEIGHT + GAP; y+= BRICK_HEIGHT + GAP)
    {
      for( uint16_t x = BRICK_WIDTH + GAP; x < OLED_WIDTH - BRICK_WIDTH+ GAP; x += BRICK_WIDTH + GAP)
      {
        bricks[brickAmount].x = x;
        bricks[brickAmount].y = y;
        hearts[brickAmount].x = x + (BRICK_WIDTH / 2); 
        hearts[brickAmount].y = y + (BRICK_HEIGHT / 2); 
        hearts[brickAmount].bottom = hearts[brickAmount].y + 5; 
        brickAmount++;        
      }
    }
    brickAmount = 0;
    chapterNumber++;
  }

  if(chapterNumber == 2)
  {
      for(int i = 0; i < MAX_BRICK_AMOUNT; i++)
      {
        if(i % 2 == 0)
        {
          Serial.println(bricks[i].isHit);
          bricks[i].isHit = false;
          hearts[i].x = bricks[i].x + (BRICK_WIDTH / 2); 
          hearts[i].y = bricks[i].y + (BRICK_HEIGHT / 2); 
          hearts[i].bottom = hearts[i].y + 5;
          hearts[i].shouldDown = false;
          brickAmount++;
        }
        else
        {
          bricks[i].isHit = true; 
        }
      }
          brickAmount = 0;
    chapterNumber++;
  }

  if(chapterNumber == 3)
  {
    // 8 9 10 11 14 15 16 17 20 21 22 23
    for(int i = 0; i < MAX_BRICK_AMOUNT; i++)
    {
      if( i == 7 || i == 8 || i == 9 || i == 10 || 
      i == 13 || i == 14 || i == 15 || i == 16 || 
      i == 19 || i == 20 || i == 21 || i == 22)
      {
        bricks[i].isHit = true;
        continue;
      }

      bricks[i].isHit = false;
      hearts[i].x = bricks[i].x + (BRICK_WIDTH / 2); 
      hearts[i].y = bricks[i].y + (BRICK_HEIGHT / 2); 
      hearts[i].bottom = hearts[i].y + 5;
      hearts[i].shouldDown = false;
      brickAmount++;
    }
              brickAmount = 0;
    chapterNumber++;
  }

  if(chapterNumber == 4)
  {
    for(int i = 0; i < MAX_BRICK_AMOUNT; i++)
    {
      if( i == 1 || i == 3 || i == 5 || i == 6 || 
      i == 8 || i == 10 || i == 13 || i == 15 || 
      i == 17 || i == 18 || i == 20 || i == 22 || 
      i == 25 || i == 27 || i == 29)
      {
        bricks[i].isHit = true;
        continue;
      }

      bricks[i].isHit = false;
      hearts[i].x = bricks[i].x + (BRICK_WIDTH / 2); 
      hearts[i].y = bricks[i].y + (BRICK_HEIGHT / 2); 
      hearts[i].bottom = hearts[i].y + 5;
      hearts[i].shouldDown = false;
      brickAmount++;
    }
  }

  if(chapterNumber == 5)
  {
    
  }
}

void updateBallSpeedForNextChapter(Ball& _ball)
{
  _ball.directionX += _ball.directionX * 0.2;
  _ball.directionY += _ball.directionY * 0.2;
}

void heartsCollisionChecks()
{
  for (int i = 0; i < MAX_BRICK_AMOUNT; i++)
  {
    hearts[i].fallDown();
    if(hearts[i].shouldDown)
    {
      hearts[i].collisionCheck(palette, updateHealthLeds);
    }
  }
}

void updateScoreBoard()
{
  int unitsDigit = palette.score % 10;
  int tensDigit = (palette.score / 10) % 10;

  switch(unitsDigit)
  {
    case 0:
      digitalWrite(SEVEN_1_A, LOW);
      digitalWrite(SEVEN_1_B, LOW);
      digitalWrite(SEVEN_1_C, LOW);
      digitalWrite(SEVEN_1_D, LOW);
      digitalWrite(SEVEN_1_E, LOW);
      digitalWrite(SEVEN_1_F, LOW);
      digitalWrite(SEVEN_1_G, HIGH);
      break;
  
    case 1:
      digitalWrite(SEVEN_1_A, HIGH);
      digitalWrite(SEVEN_1_B, LOW);
      digitalWrite(SEVEN_1_C, LOW);
      digitalWrite(SEVEN_1_D, HIGH);
      digitalWrite(SEVEN_1_E, HIGH);
      digitalWrite(SEVEN_1_F, HIGH);
      digitalWrite(SEVEN_1_G, HIGH);
      break;
      
    case 2:
      digitalWrite(SEVEN_1_A, LOW);
      digitalWrite(SEVEN_1_B,LOW);
      digitalWrite(SEVEN_1_C,HIGH);
      digitalWrite(SEVEN_1_D,LOW);
      digitalWrite(SEVEN_1_E,LOW);
      digitalWrite(SEVEN_1_F,HIGH);
      digitalWrite(SEVEN_1_G,LOW);
      break;
      
    case 3:
      digitalWrite(SEVEN_1_A, LOW);
      digitalWrite(SEVEN_1_B,LOW);
      digitalWrite(SEVEN_1_C,LOW);
      digitalWrite(SEVEN_1_D,LOW);
      digitalWrite(SEVEN_1_E,HIGH);
      digitalWrite(SEVEN_1_F,HIGH);
      digitalWrite(SEVEN_1_G,LOW);
      break;
      
    case 4:
      digitalWrite(SEVEN_1_A, HIGH);
      digitalWrite(SEVEN_1_B,LOW);
      digitalWrite(SEVEN_1_C,LOW);
      digitalWrite(SEVEN_1_D,HIGH);
      digitalWrite(SEVEN_1_E,HIGH);
      digitalWrite(SEVEN_1_F,LOW);
      digitalWrite(SEVEN_1_G,LOW);
      break;
      
    case 5:
      digitalWrite(SEVEN_1_A, LOW);
      digitalWrite(SEVEN_1_B,HIGH);
      digitalWrite(SEVEN_1_C,LOW);
      digitalWrite(SEVEN_1_D,LOW);
      digitalWrite(SEVEN_1_E,HIGH);
      digitalWrite(SEVEN_1_F,LOW);
      digitalWrite(SEVEN_1_G,LOW);
      break;
      
    case 6:
      digitalWrite(SEVEN_1_A, LOW);
      digitalWrite(SEVEN_1_B,HIGH);
      digitalWrite(SEVEN_1_C,LOW);
      digitalWrite(SEVEN_1_D,LOW);
      digitalWrite(SEVEN_1_E,LOW);
      digitalWrite(SEVEN_1_F,LOW);
      digitalWrite(SEVEN_1_G,LOW);
      break;
      
    case 7:
      digitalWrite(SEVEN_1_A, LOW);
      digitalWrite(SEVEN_1_B,LOW);
      digitalWrite(SEVEN_1_C,LOW);
      digitalWrite(SEVEN_1_D,HIGH);
      digitalWrite(SEVEN_1_E,HIGH);
      digitalWrite(SEVEN_1_F,HIGH);
      digitalWrite(SEVEN_1_G,HIGH);
      break;
  
    case 8:
      digitalWrite(SEVEN_1_A, LOW);
      digitalWrite(SEVEN_1_B, LOW);
      digitalWrite(SEVEN_1_C, LOW);
      digitalWrite(SEVEN_1_D, LOW);
      digitalWrite(SEVEN_1_E, LOW);
      digitalWrite(SEVEN_1_F, LOW);
      digitalWrite(SEVEN_1_G, LOW);
      break;
      
    case 9:
      digitalWrite(SEVEN_1_A,LOW);
      digitalWrite(SEVEN_1_B,LOW);
      digitalWrite(SEVEN_1_C,LOW);
      digitalWrite(SEVEN_1_D,LOW);
      digitalWrite(SEVEN_1_E,HIGH);
      digitalWrite(SEVEN_1_F,LOW);
      digitalWrite(SEVEN_1_G,LOW);
      break;
  }

  switch(tensDigit)
  {
      case 0:
      digitalWrite(SEVEN_2_A, LOW);
      digitalWrite(SEVEN_2_B, LOW);
      digitalWrite(SEVEN_2_C, LOW);
      digitalWrite(SEVEN_2_D, LOW);
      digitalWrite(SEVEN_2_E, LOW);
      digitalWrite(SEVEN_2_F, LOW);
      digitalWrite(SEVEN_2_G, HIGH);
      break;
  
    case 1:
      digitalWrite(SEVEN_2_A, HIGH);
      digitalWrite(SEVEN_2_B, LOW);
      digitalWrite(SEVEN_2_C, LOW);
      digitalWrite(SEVEN_2_D, HIGH);
      digitalWrite(SEVEN_2_E, HIGH);
      digitalWrite(SEVEN_2_F, HIGH);
      digitalWrite(SEVEN_2_G, HIGH);
      break;
      
    case 2:
      digitalWrite(SEVEN_2_A, LOW);
      digitalWrite(SEVEN_2_B,LOW);
      digitalWrite(SEVEN_2_C,HIGH);
      digitalWrite(SEVEN_2_D,LOW);
      digitalWrite(SEVEN_2_E,LOW);
      digitalWrite(SEVEN_2_F,HIGH);
      digitalWrite(SEVEN_2_G,LOW);
      break;
      
    case 3:
      digitalWrite(SEVEN_2_A, LOW);
      digitalWrite(SEVEN_2_B,LOW);
      digitalWrite(SEVEN_2_C,LOW);
      digitalWrite(SEVEN_2_D,LOW);
      digitalWrite(SEVEN_2_E,HIGH);
      digitalWrite(SEVEN_2_F,HIGH);
      digitalWrite(SEVEN_2_G,LOW);
      break;
      
    case 4:
      digitalWrite(SEVEN_2_A, HIGH);
      digitalWrite(SEVEN_2_B,LOW);
      digitalWrite(SEVEN_2_C,LOW);
      digitalWrite(SEVEN_2_D,HIGH);
      digitalWrite(SEVEN_2_E,HIGH);
      digitalWrite(SEVEN_2_F,LOW);
      digitalWrite(SEVEN_2_G,LOW);
      break;
      
    case 5:
      digitalWrite(SEVEN_2_A, LOW);
      digitalWrite(SEVEN_2_B,HIGH);
      digitalWrite(SEVEN_2_C,LOW);
      digitalWrite(SEVEN_2_D,LOW);
      digitalWrite(SEVEN_2_E,HIGH);
      digitalWrite(SEVEN_2_F,LOW);
      digitalWrite(SEVEN_2_G,LOW);
      break;
      
    case 6:
      digitalWrite(SEVEN_2_A, LOW);
      digitalWrite(SEVEN_2_B,HIGH);
      digitalWrite(SEVEN_2_C,LOW);
      digitalWrite(SEVEN_2_D,LOW);
      digitalWrite(SEVEN_2_E,LOW);
      digitalWrite(SEVEN_2_F,LOW);
      digitalWrite(SEVEN_2_G,LOW);
      break;
      
    case 7:
      digitalWrite(SEVEN_2_A, LOW);
      digitalWrite(SEVEN_2_B,LOW);
      digitalWrite(SEVEN_2_C,LOW);
      digitalWrite(SEVEN_2_D,HIGH);
      digitalWrite(SEVEN_2_E,HIGH);
      digitalWrite(SEVEN_2_F,HIGH);
      digitalWrite(SEVEN_2_G,HIGH);
      break;
  
    case 8:
      digitalWrite(SEVEN_2_A, LOW);
      digitalWrite(SEVEN_2_B, LOW);
      digitalWrite(SEVEN_2_C, LOW);
      digitalWrite(SEVEN_2_D, LOW);
      digitalWrite(SEVEN_2_E, LOW);
      digitalWrite(SEVEN_2_F, LOW);
      digitalWrite(SEVEN_2_G, LOW);
      break;
      
    case 9:
      digitalWrite(SEVEN_2_A,LOW);
      digitalWrite(SEVEN_2_B,LOW);
      digitalWrite(SEVEN_2_C,LOW);
      digitalWrite(SEVEN_2_D,LOW);
      digitalWrite(SEVEN_2_E,HIGH);
      digitalWrite(SEVEN_2_F,LOW);
      digitalWrite(SEVEN_2_G,LOW);
      break;
  }
}

void updateHealthLeds()
{
  switch(palette.currentHealth)
  {
    case 0:
      digitalWrite(LOW_HEALTH_LED, LOW);
      digitalWrite(MID_HEALTH_LED, LOW);
      digitalWrite(HIGH_HEALTH_LED, LOW);
      break;
    case 1:
      digitalWrite(LOW_HEALTH_LED, HIGH);
      digitalWrite(MID_HEALTH_LED, LOW);
      digitalWrite(HIGH_HEALTH_LED, LOW);
      break;
    case 2:
      digitalWrite(LOW_HEALTH_LED, HIGH);
      digitalWrite(MID_HEALTH_LED, HIGH);
      digitalWrite(HIGH_HEALTH_LED, LOW);
      break;
    case 3:
      digitalWrite(LOW_HEALTH_LED, HIGH);
      digitalWrite(MID_HEALTH_LED, HIGH);
      digitalWrite(HIGH_HEALTH_LED, HIGH);
      break;
  }
}

void setup() 
{
  randomSeed(analogRead(A2));
  digitalWrite(JOY_BUTTON, HIGH);
  initVariables();
  initBegins();
  setLevel();
  updateScoreBoard();
  updateHealthLeds();
}

void loop() 
{
  display.clearDisplay();
  if(!bIsGameStart && !bIsGameOver && !bIsGameQuit && !bIsGameWin)
  {
    openingScreen();
  }

  else if(bIsGameStart && !bIsGameOver && !bIsGameQuit && !bIsGameWin)
  {
    if(bIsReborn)
    {
      reborn();
      bIsReborn = false;
    }

    palette.paletteMove();
    drawPalette();
    ball.ballMove(palette);
    ball.collisionChecks(palette, updateHealthLeds, updateScoreBoard, shouldHeartDown, hearts, bricks);
    heartsCollisionChecks();
    drawBall(); 
    drawBricks();
    drawHealth();
    //Serial.println(palette.currentHealth);
  }

  else if(bIsGameWin)
  {
    winScreen();
  }

  else if(!bIsGameStart && !bIsGameOver && bIsGameQuit && !bIsGameWin)
  {
    quitScreen();
  }
  
  else if(bIsGameOver)
  {
    gameOverScreen(); 
    if(digitalRead(JOY_BUTTON) == LOW)
    {
      backToMenu();    
    }
  }

  display.display();
}
