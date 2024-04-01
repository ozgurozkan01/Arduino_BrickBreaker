#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define OLED_ADDR 0x3C

#define GAP 1
#define MAX_BRICK_AMOUNT 40

#define joyX A0
#define joyY A1
#define joyButton 2

#define upButton 3
#define downButton 4
#define selectionButton 5

#define BRICK_WIDTH 12
#define BRICK_HEIGHT 4

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
};

struct Ball
{
  float radius;
  float x;
  float y;
  float directionX; // - -> left, + -> rigth
  float directionY; // -1 -> up, 1 -> down
};

Ball ball;
Palette palette;
uint8_t brickCoords[MAX_BRICK_AMOUNT][2] =  {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
                                        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
                                        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
                                        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},};

bool isBrickHitList[MAX_BRICK_AMOUNT] =  {false, false, false, false, false, false, false, false, false, false,
                                        false, false, false, false, false, false, false, false, false, false,
                                        false, false, false, false, false, false, false, false, false, false,
                                        false, false, false, false, false, false, false, false, false, false};
uint8_t brickAmount;
uint8_t brickIndex;

bool isGameStart;
bool isGameOver;
bool isGameQuit;

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

enum OpeningOptions
{
  START,
  QUIT
};

OpeningOptions currentOption;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT);

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

    isGameStart = false;
    isGameOver = false;
    isGameQuit = false;
    
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
    float initdirectionX = random(0, 21);
    ball.directionX = initdirectionX / 10 - 1;
    ball.directionY = -1.f; // -1 -> up, 1 -> down

    selectionButtonPrev = LOW;
    upButtonPrev = LOW;
    downButtonPrev = LOW;

    currentOption = OpeningOptions::START;
    brickAmount = 0;
    brickIndex = 0;
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
        isGameStart = true;
        break;
      case OpeningOptions::QUIT:
        isGameQuit = true;
        break;
    }
  }
  
  selectionButtonPrev = digitalRead(selectionButton);
}

// Ekrandaki tuglalari olusturur
void createBricks()
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
        if(isBrickHitList[index] == false)
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

void paletteMove()
{ 
    //int xValue = analogRead(joyX);
    int yValue = analogRead(joyY);

    if (yValue < 10 && palette.paletteX < OLED_WIDTH - palette.paletteWidth) 
    {
      palette.paletteX++;
      palette.paletteLeftSide++;
      palette.paletteRightSide++;    
    }
    
    if (yValue > 1000 && palette.paletteX > 0) 
    {
      palette.paletteX--;
      palette.paletteLeftSide--;
      palette.paletteRightSide--;    
    }
}

void ballMove()
{
    ball.y += ball.directionY;
    ball.x += ball.directionX;
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
        isGameOver = true;           
        return;
      }
      restartGame();     
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
      if( isBrickHitList[i] == false && 
          ball.x <= brickCoords[i][0] + BRICK_WIDTH && // LEFT BORDER 
          ball.x >= brickCoords[i][0] && // RIGTH BORDER
          ((ball.y > brickCoords[i][1] && ball.y - brickCoords[i][1] <= ball.radius + BRICK_HEIGHT) || // DOWN BORDER
           (ball.y < brickCoords[i][1] && ball.y - brickCoords[i][1] >= -ball.radius)) // UP BORDER
        )
      {
        isBrickHitList[i] = true;
        ball.directionY *= -1;
        palette.score++;
        Serial.println(palette.score);
      }


      else if( isBrickHitList[i] == false && 
          ball.y <= brickCoords[i][1] + BRICK_HEIGHT &&
          ball.y >= brickCoords[i][1] &&
          ((ball.x > brickCoords[i][0] && ball.x - brickCoords[i][0] <= ball.radius + BRICK_WIDTH) || 
           (ball.x < brickCoords[i][0] && ball.x - brickCoords[i][0] >= -ball.radius))
        )
      {
        isBrickHitList[i] = true;
        ball.directionX *= -1;
        palette.score++;
        Serial.println(palette.score);
      }
    }
}

void breakBricks()
{
  
}

void gameOverScreen()
{
  // GAME OVER TEXT
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.println("GAME OVER");
  // RESTART TEXT
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setCursor(restartTextX, 40);
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

void restartGame()
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
  isGameOver = false;
  initVariables();
}

void setup() 
{
  randomSeed(analogRead(A2));
  digitalWrite(joyButton, HIGH);
  initVariables();
  initBegins();
  createBricks();
}

void loop() {

  display.clearDisplay();

  if(!isGameStart && !isGameOver && !isGameQuit)
  {
    openingScreen();
  }

  else if(isGameStart && !isGameOver && !isGameQuit)
  {
    drawBricks();
    paletteMove();
    drawPalette();
    ballMove();
    ballCollisionChecks();
    drawBall();
  }

  else if(!isGameStart && !isGameOver && isGameQuit)
  {
    quitScreen();
  }
  
  else if(isGameOver)
  {
    gameOverScreen(); 
    if(digitalRead(joyButton) == LOW)
    {
      backToMenu();    
    }
  }
  display.display();
}
