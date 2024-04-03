#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define OLED_ADDR 0x3C

#define GAP 1
#define MAX_BRICK_AMOUNT 30

#define joyX A0
#define joyY A1
#define joyButton 2

#define upButton 3
#define downButton 4
#define selectionButton 5

#define BRICK_WIDTH 16
#define BRICK_HEIGHT 4

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT);

struct Palette
{
  uint8_t maxHealth;
  uint8_t currentHealth;
  uint8_t paletteHeight;
  uint8_t paletteWidth;
  uint8_t paletteX;
  uint8_t paletteY;
  uint8_t paletteUpSide;
  uint8_t paletteLeftSide;
  uint8_t paletteRightSide;
  uint8_t score;

    void paletteMove()
  { 
      //int xValue = analogRead(joyX);
      int yValue = analogRead(joyY);
  
      if (yValue < 10 && paletteX < OLED_WIDTH - paletteWidth) 
      {
        paletteX++;
        paletteLeftSide++;
        paletteRightSide++;    
      }
      
      if (yValue > 1000 && paletteX > 0) 
      {
        paletteX--;
        paletteLeftSide--;
        paletteRightSide--;    
      }
  }
};

struct Ball
{
  float radius;
  float x;
  float y;
  float directionX; // - -> left, + -> rigth
  float directionY; // -1 -> up, 1 -> down

  void ballMove()
  {
      y += directionY;
      x += directionX;
  }
};

struct Health
{
  int16_t circleRadius = 2;
  int16_t coverCircleRadius = 8;
  int16_t coverCircleX;
  int16_t coverCircleY;
  int16_t leftCircleY;
  int16_t rightCircleY;
  int16_t triangle_Y_leftCorner;
  int16_t triangle_Y_rightCorner;
  int16_t triangle_Y_middleCorner;

  void initializeHealthPos(int16_t _coverCircleX, int16_t _coverCircleY)
  {
    coverCircleX = _coverCircleX;
    coverCircleY = _coverCircleY;
    leftCircleY = _coverCircleY - 1;
    rightCircleY = _coverCircleY - 1;
    triangle_Y_leftCorner = _coverCircleY - 1;
    triangle_Y_rightCorner = _coverCircleY - 1;
    triangle_Y_middleCorner = _coverCircleY - 1 + 5;
  }

  void fallDownHealth()
  {
    coverCircleY++;
    leftCircleY++;
    rightCircleY++;
    triangle_Y_leftCorner++;
    triangle_Y_rightCorner++;
    triangle_Y_middleCorner++;
  }
};

Health health;
Ball ball;
Palette palette;
uint8_t brickCoords[MAX_BRICK_AMOUNT][2] =  {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
                                             {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
                                             {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};

bool brickDisableOrderList[MAX_BRICK_AMOUNT] =  {false, false, false, false, false, false, false, false, false, false,
                                                 false, false, false, false, false, false, false, false, false, false,
                                                 false, false, false, false, false, false, false, false, false, false};

uint8_t brickAmount;
uint8_t brickIndex;

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

uint8_t selectionButtonPrev;
uint8_t upButtonPrev; // prevent running in each frame when button is staying pressed
uint8_t downButtonPrev; // prevent running in each frame when button is staying pressed

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
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
}

void initVariables()
{   
    pinMode(upButton, INPUT);
    pinMode(downButton, INPUT);
    pinMode(selectionButton, INPUT);

    bIsGameStart = false;
    bIsGameOver = false;
    bIsGameQuit = false;
    bIsReborn = false;
    bIsGameWin = false;
    bShouldHealthFallDown = false;
    
    selectionCursorX = 45;
    selectionCursorY = 30;

    palette.maxHealth = 3;
    palette.currentHealth = palette.maxHealth;
    palette.paletteWidth = 20;
    palette.paletteHeight = 3;
    palette.paletteX = OLED_WIDTH / 2 - palette.paletteWidth / 2;
    palette.paletteY = OLED_HEIGHT - palette.paletteHeight;
    palette.paletteUpSide = palette.paletteY;
    palette.paletteLeftSide = palette.paletteX;
    palette.paletteRightSide = palette.paletteX + palette.paletteWidth;
    palette.score = 0;

    ball.radius = 1;
    ball.x = palette.paletteX + palette.paletteWidth / 2;
    ball.y = palette.paletteY - 2 * ball.radius;
    float initdirectionX = random(0, 31);
    ball.directionX = (initdirectionX / 10 - 1);
    ball.directionY = -1.5f; // -1 -> up, 1 -> down

    selectionButtonPrev = LOW;
    upButtonPrev = LOW;
    downButtonPrev = LOW;

    currentOption = OpeningOptions::START;
    brickAmount = 0;
    brickIndex = 0;

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
  if(digitalRead(upButton) == HIGH && upButtonPrev == LOW && openingScreenOptionIndex != 0)
  {
    openingScreenOptionIndex--;
    currentOption = (OpeningOptions)openingScreenOptionIndex;
    selectionCursorY = openingScreenOptionsY[openingScreenOptionIndex];
  }
  upButtonPrev = digitalRead(upButton);
  
  // MOVING DOWN THROUGH THE OPTIONS
  if(digitalRead(downButton) == HIGH && downButtonPrev == LOW && openingScreenOptionIndex != 1)
  {
    openingScreenOptionIndex++;
    currentOption = (OpeningOptions)openingScreenOptionIndex;
    selectionCursorY = openingScreenOptionsY[openingScreenOptionIndex];
  }
  downButtonPrev = digitalRead(downButton);
  
  // CHOOSE CURRENT OPTION
  if(digitalRead(selectionButton) == HIGH && selectionButtonPrev == LOW)
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
  
  selectionButtonPrev = digitalRead(selectionButton);
}

// Ekrandaki tuglalari olusturur
void initializeBricksPosition()
{
    for (uint16_t y = BRICK_HEIGHT + GAP; y < OLED_HEIGHT / 2 - BRICK_HEIGHT + GAP; y+= BRICK_HEIGHT + GAP)
    {
      for( uint16_t x = BRICK_WIDTH + GAP; x < OLED_WIDTH - BRICK_WIDTH+ GAP; x += BRICK_WIDTH + GAP)
      {
        brickCoords[brickAmount][0] = x;
        brickCoords[brickAmount][1] = y;
        brickAmount++;        
      }
    }
} 

void drawBricks()
{
  uint8_t index = 0;
    for (uint16_t y = BRICK_HEIGHT + GAP; y < OLED_HEIGHT / 2 - BRICK_HEIGHT + GAP; y+= BRICK_HEIGHT + GAP)
    {
      for( uint16_t x = BRICK_WIDTH+ GAP; x < OLED_WIDTH - BRICK_WIDTH + GAP; x += BRICK_WIDTH + GAP)
      {
        if(brickDisableOrderList[index] == false)
        {
          display.fillRect(x, y, BRICK_WIDTH, BRICK_HEIGHT, WHITE); 
        }
        index++;
      }
    }
}

void drawPalette()
{
    display.fillRect(palette.paletteX, palette.paletteY, palette.paletteWidth, palette.paletteHeight, WHITE);
}

void drawBall()
{
    display.fillCircle(ball.x, ball.y, ball.radius, WHITE);
}

/*void drawHealth(Health& _health)
{
    display.drawCircle(_health.coverCircleX, _health.coverCircleY, _health.coverCircleRadius, WHITE);
    display.fillCircle(_health.coverCircleX - _health.circleRadius, _health.coverCircleY - 1, _health.circleRadius, WHITE);
    display.fillCircle(_health.coverCircleX + _health.circleRadius, _health.coverCircleY - 1, _health.circleRadius, WHITE);
    
    display.fillTriangle(_health.coverCircleX - _health.circleRadius - _health.circleRadius, _health.coverCircleY - 1,
                          _health.coverCircleX + _health.circleRadius + _health.circleRadius, _health.coverCircleY - 1,
                          _health.coverCircleX, _health.coverCircleY - 1 + 5,  WHITE);
}*/

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
    if(ball.x <= palette.paletteRightSide && ball.x >= palette.paletteLeftSide && ball.y + ball.radius >= palette.paletteUpSide)
    {
      ball.directionY *= -1;
    }
    // BRICKS COLLISION CHECK
    for(int i = 0; i < brickAmount; i++)
    {
      // UP AND DOWN BORDER CHECK
      if( brickDisableOrderList[i] == false && 
          ball.x <= brickCoords[i][0] + BRICK_WIDTH && // LEFT BORDER 
          ball.x >= brickCoords[i][0] && // RIGTH BORDER
          ((ball.y > brickCoords[i][1] && ball.y - brickCoords[i][1] <= ball.radius + BRICK_HEIGHT) || // DOWN BORDER
           (ball.y < brickCoords[i][1] && ball.y - brickCoords[i][1] >= -ball.radius)) // UP BORDER
        )
      {
        brickDisableOrderList[i] = true;
        ball.directionY *= -1;
        palette.score++;
        //bShouldHealthFallDown = shouldHealthFallDown();
      }


      else if( brickDisableOrderList[i] == false && 
          ball.y <= brickCoords[i][1] + BRICK_HEIGHT &&
          ball.y >= brickCoords[i][1] &&
          ((ball.x > brickCoords[i][0] && ball.x - brickCoords[i][0] <= ball.radius + BRICK_WIDTH) || 
           (ball.x < brickCoords[i][0] && ball.x - brickCoords[i][0] >= -ball.radius))
        )
      {
        brickDisableOrderList[i] = true;
        ball.directionX *= -1;
        palette.score++; 
        //bShouldHealthFallDown = shouldHealthFallDown();
      }

      if(palette.score == brickAmount)
      {
        bIsGameWin = true;
      }
    }
}

bool shouldHealthFallDown()
{
  int healthDownRate = random(0, 101);
  if(healthDownRate >= 90)
  {
    return true;
  }

  return false;
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
    palette.paletteX = OLED_WIDTH / 2 - palette.paletteWidth / 2;
    palette.paletteY = OLED_HEIGHT - palette.paletteHeight;
    palette.paletteUpSide = palette.paletteY;
    palette.paletteLeftSide = palette.paletteX;
    palette.paletteRightSide = palette.paletteX + palette.paletteWidth;

    ball.x = palette.paletteX + palette.paletteWidth / 2;
    ball.y = palette.paletteY - 2 * ball.radius;
    float initdirectionX = random(0, 21);
    ball.directionX = initdirectionX / 10 - 1;
    ball.directionY = -1.f; // -1 -> up, 1 -> down
}

void backToMenu()
{
  restartTextX = OLED_WIDTH;
  bIsGameOver = false;
  initVariables();

  initializeBricksPosition();

  for(int i = 0; i < brickAmount; i++)
  {
    brickDisableOrderList[i] = false;
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
          brickDisableOrderList[i] = false;
          brickAmount++;
        }
        else
        {
            brickDisableOrderList[i] = true; 
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
        brickDisableOrderList[i] = true;
        continue;
      }
      brickDisableOrderList[i] = false;
      brickAmount++;
    }
  }

  Serial.println(brickAmount);
}

void updateBallSpeedForNextChapter(Ball& _ball)
{
  _ball.directionX += _ball.directionX * 0.2;
  _ball.directionY += _ball.directionY * 0.2;
}

void setup() 
{
  randomSeed(analogRead(A2));
  digitalWrite(joyButton, HIGH);
  initVariables();
  initBegins();
  initializeBricksPosition();
  health.initializeHealthPos(62, 31);
}

void loop() 
{
  display.clearDisplay();
  
  if(!bIsGameStart && !bIsGameOver && !bIsGameQuit)
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

    if(bShouldHealthFallDown)
    {
      //drawHealth(health);
      //health.fallDownHealth();
    }
    
    palette.paletteMove();
    ball.ballMove();
    ballCollisionChecks();
    drawBall(); 
    drawBricks();
    drawPalette();
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
    if(digitalRead(joyButton) == LOW)
    {
      backToMenu();    
    }
  }
  display.display();
}
