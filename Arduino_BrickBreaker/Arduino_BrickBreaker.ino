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

typedef struct
{
  uint8_t maxHealth = 3;
  int8_t currentHealth = 3;
  uint8_t height = 3;
  uint8_t width = 20;
  uint8_t x;
  uint8_t y;
  uint8_t score = 0;

  void paletteMove()
  { 
    //int xValue = analogRead(JOY_X);
    int yValue = analogRead(JOY_Y);
  
    if (yValue < 10 && x < OLED_WIDTH - width) 
    {
      x++; 
    }
      
    if (yValue > 1000 && x > 0) 
    {
      x--;   
    }
  }  
} Palette;

typedef struct
{
  uint8_t radius = 2;
  float x;
  float y;
  float directionX; // - -> left, + -> rigth
  float directionY; // -1 -> up, 1 -> down

  void ballMove()
  {
      y += directionY;
      x += directionX;
  }
} Ball;

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

  void collisionCheck(Palette& _palette)
  {
    if (_palette.x < x && _palette.x + _palette.width > x && _palette.y <= bottom)
    {
      if (_palette.currentHealth < 3)
      {
        _palette.currentHealth++;
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

Ball ball;
Palette palette;

Brick bricks[MAX_BRICK_AMOUNT];
Heart hearts[MAX_BRICK_AMOUNT];

uint8_t brickAmount;

bool bIsGameStart;
bool bIsGameOver;
bool bIsGameQuit;
bool bIsGameWin;
bool bIsReborn;
bool bShouldHealthFallDown;

int16_t restartTextX = OLED_WIDTH;
int16_t quitTextX = OLED_WIDTH;

// Cursor positions in opening screen to choose option
uint8_t selectionCursorY;
uint8_t selectionCursorX;

uint8_t SELECTION_BUTTONPrev;
uint8_t UP_BUTTONPrev; // prevent running in each frame when button is staying pressed
uint8_t DOWN_BUTTONPrev; // prevent running in each frame when button is staying pressed

uint8_t openingScreenOptionIndex = 0;
uint8_t openingScreenOptionsY[2]= {30, 40};

int8_t nextChapterTimer;
uint8_t chapterNumber;

enum OpeningOptions
{
  START,
  QUIT
};

OpeningOptions currentOption;

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

    bIsGameStart = false;
    bIsGameOver = false;
    bIsGameQuit = false;
    bIsReborn = false;
    bIsGameWin = false;
    bShouldHealthFallDown = false;
    
    selectionCursorX = 45;
    selectionCursorY = 30;

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
    brickAmount = 0;
    nextChapterTimer = 3;
    chapterNumber = 0;
}

void openingScreen()
{
  // GAME NAME
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(25, 10);
  display.println("BRICK BREAKER");
  // START TEXT
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 30);
  display.println("START");
  // QUIT TEXT
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 40);
  display.println("QUIT");
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
  }
  UP_BUTTONPrev = digitalRead(UP_BUTTON);
  
  // MOVING DOWN THROUGH THE OPTIONS
  if(digitalRead(DOWN_BUTTON) == HIGH && DOWN_BUTTONPrev == LOW && openingScreenOptionIndex != 1)
  {
    openingScreenOptionIndex++;
    currentOption = (OpeningOptions)openingScreenOptionIndex;
    selectionCursorY = openingScreenOptionsY[openingScreenOptionIndex];
  }
  DOWN_BUTTONPrev = digitalRead(DOWN_BUTTON);
  
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
        break;
    }
  }
  
  SELECTION_BUTTONPrev = digitalRead(SELECTION_BUTTON);
}

// Ekrandaki tuglalari olusturur
void initializeBricksPosition()
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
} 

void drawBricks()
{
  for (int i = 0; i < brickAmount; i++)
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
  for (int i = 0; i < brickAmount; i++)
  {
     if(hearts[i].shouldDown == true)
     {
           display.fillCircle(hearts[i].x - 2, hearts[i].y, 2, WHITE);
           display.fillCircle(hearts[i].x + 2, hearts[i].y, 2, WHITE);
           display.fillTriangle(hearts[i].x - 4, hearts[i].y, hearts[i].x + 4, hearts[i].y, hearts[i].x , hearts[i].y + 5, WHITE);
     }
  }
}

void ballCollisionChecks()
{    
    // LEFT AND RIGHT SCREEN BORDER COLLISION CHECK
    if(ball.x < ball.radius || ball.x > OLED_WIDTH - ball.radius)
    {
      ball.directionX *= -1;  

    }

    // UP SCREEN BORDER COLLISION CHECK
    if(ball.y < ball.radius)
    {
      ball.directionY *= -1;
    }
    // DOWN SCREEN BORDER COLLISION CHECK
    if(ball.y > OLED_HEIGHT - ball.radius)
    {
      palette.currentHealth--;
      if(palette.currentHealth <= 0)
      {
        bIsGameOver = true;
        delay(3000);       
        return;
      }
      bIsReborn = true;     
    }
    
    // PALETTE COLISION CHECK
    if(ball.x <= palette.x + palette.width && ball.x >= palette.x && ball.y + ball.radius >= palette.y)
    {
      ball.directionY *= -1;
    }
    // BRICKS COLLISION CHECK
    for(int i = 0; i < brickAmount; i++)
    {
      // UP AND DOWN BORDER CHECK
      if( bricks[i].isHit == false && 
          ball.x <= bricks[i].x + BRICK_WIDTH && // LEFT BORDER 
          ball.x >= bricks[i].x && // RIGTH BORDER
          ((ball.y > bricks[i].y && ball.y - bricks[i].y <= ball.radius + BRICK_HEIGHT) || // DOWN BORDER
           (ball.y < bricks[i].y && ball.y - bricks[i].y >= -ball.radius)) // UP BORDER
        )
      {
        bricks[i].isHit = true;
        ball.directionY *= -1;
        palette.score++;
        scoreBoardUpdate();
        hearts[i].shouldDown = shouldHeartDown();
      }


      else if( bricks[i].isHit == false && 
          ball.y <= bricks[i].y + BRICK_HEIGHT &&
          ball.y >= bricks[i].y &&
          ((ball.x > bricks[i].x && ball.x - bricks[i].x <= ball.radius + BRICK_WIDTH) || 
           (ball.x < bricks[i].x && ball.x - bricks[i].x >= -ball.radius))
        )
      {
        bricks[i].isHit = true;
        ball.directionX *= -1;
        palette.score++; 
        scoreBoardUpdate();
        hearts[i].shouldDown = shouldHeartDown();
      }

      if(palette.score == brickAmount)
      {
        bIsGameWin = true;
      }
    }
}

bool shouldHeartDown()
{
  return random(0, 101) > 40;
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
    palette.x = OLED_WIDTH / 2 - palette.width / 2;
    palette.y = OLED_HEIGHT - palette.height;

    ball.x = palette.x + palette.width / 2;
    ball.y = palette.y - 2 * ball.radius;
    float initdirectionX = random(0, 21);
    ball.directionX = initdirectionX / 10 - 1;
    ball.directionY = -1.f; // -1 -> up, 1 -> down
}

void backToMenu()
{
  restartTextX = OLED_WIDTH;
  bIsGameOver = false;
  palette.score = 0;

  initVariables();
  initializeBricksPosition();

  for(int i = 0; i < brickAmount; i++)
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
      palette.score = 0;
      nextChapterTimer = 3;
      palette.currentHealth = 3;
      updateBricksForNewChapter();
      updateBallSpeedForNextChapter(ball);
      reborn();
      bIsGameWin = false;
  }
}

void updateBricksForNewChapter()
{
  brickAmount = 0;
  chapterNumber++;
  
  if(chapterNumber == 1)
  {
      for(int i = 0; i < MAX_BRICK_AMOUNT; i++)
      {
        if(i % 2 == 0)
        {
          bricks[i].isHit = false;
          brickAmount++;
        }
        else
        {
            bricks[i].isHit = true; 
        }
      }
  }

  else if(chapterNumber == 2)
  {
    // 8 9 10 11 14 15 16 17 20 21 22 23
    for(int i = 0; i < MAX_BRICK_AMOUNT; i++)
    {
      if( i == 8 && i == 9 && i == 10 && i == 11 && 
      i == 14 && i == 15 && i == 16 && i == 17 && 
      i == 20 && i == 21 && i == 22 && i == 23)
      {
        bricks[i].isHit = true;
        continue;
      }

      bricks[i].isHit = false;
      brickAmount++;
    }
  }
}

void updateBallSpeedForNextChapter(Ball& _ball)
{
  _ball.directionX += _ball.directionX * 0.2;
  _ball.directionY += _ball.directionY * 0.2;
}

void heartsCollisionChecks()
{
  for (int i = 0; i < brickAmount; i++)
  {
    hearts[i].fallDown();
    if(hearts[i].shouldDown)
    {
      hearts[i].collisionCheck(palette);
    }
  }
}

void scoreBoardUpdate()
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

void setup() 
{
  randomSeed(analogRead(A2));
  digitalWrite(JOY_BUTTON, HIGH);
  initVariables();
  initBegins();
  initializeBricksPosition();
  scoreBoardUpdate();
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
    ball.ballMove();
    ballCollisionChecks();
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
