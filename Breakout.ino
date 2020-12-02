#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LEFT_BUTTON 4
#define RIGHT_BUTTON 5

#define NUM_BLOCKS_LR 5
#define NUM_BLOCKS_UD 3
#define BLOCK_PADDING 3
#define CIRCLE_RADIUS 3

#define PADDLE_POSITION_Y (display.height()-BLOCK_PADDING)

#define INFLUENCE_X 0.75

#define MAX_SPEED 50

#define BLOCK_WIDTH (display.width()/NUM_BLOCKS_LR - BLOCK_PADDING)
#define PADDLE_WIDTH BLOCK_WIDTH

#define BLOCK_HEIGHT ((display.height()/2)/NUM_BLOCKS_UD - BLOCK_PADDING)
#define PADDLE_HEIGHT BLOCK_HEIGHT

typedef struct {
  double x = 0;
  double y = 0;
  double vel_x = 0;
  double vel_y = 3;
} Ball;

int paddle_x;
Ball ball;

typedef struct {
  int x = 0;
  int y = 0;
  bool active = true;
  bool filled = true;
} Block;

Block blocks[NUM_BLOCKS_UD][NUM_BLOCKS_LR];

void setup() {
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);
  
  display.clearDisplay();
  paddle_x = (display.width()/2) - (display.width()/NUM_BLOCKS_LR - BLOCK_PADDING)/2;
  ball.x = (double)((display.width()/2));
  ball.y = (double)(PADDLE_POSITION_Y - 6);

  /* Reset board */
  for(int i = 0; i < NUM_BLOCKS_UD; i++) {
    for(int j = 0; j < NUM_BLOCKS_LR; j++) {
      blocks[i][j].active = true;
      blocks[i][j].x = j * (display.width()/NUM_BLOCKS_LR);
      blocks[i][j].y = i * ((display.height()/2)/NUM_BLOCKS_UD);
      blocks[i][j].filled = (i+j) % 2 == 0;
    }
  }
}

void loop() {
  int i, j;
  double speedXY, posX;
  /******** GAME LOGIC ********/
  
  if(leftButton()) {
    paddle_x -= 5;
  } 
  if(rightButton()) {
    paddle_x += 5;
  }

  ball.vel_y = abs(ball.vel_y) > MAX_SPEED ? 
      MAX_SPEED * (ball.vel_y > 0 ? -1 : 1) : 
      ball.vel_y;
      
  ball.vel_x = abs(ball.vel_x) > MAX_SPEED ? 
      MAX_SPEED * (ball.vel_x > 0 ? -1 : 1) : 
      ball.vel_x;
  
  speedXY = sqrt(ball.vel_x*ball.vel_x + ball.vel_y*ball.vel_y);
  
  /* Collision with wall! */
  if (ball.x - CIRCLE_RADIUS < 0 || ball.x + CIRCLE_RADIUS > display.width()) {
    ball.vel_x *= -1;
  }
  if (ball.y - CIRCLE_RADIUS < 0) {
    ball.vel_y *= -1;
  }
  if(ball.y + CIRCLE_RADIUS > display.height()) {
    ball.vel_y *= -1;
    Serial.println("Game over!");
  }
  /* Collision with paddle! */
  if (((ball.x - CIRCLE_RADIUS) - paddle_x <= PADDLE_WIDTH && (ball.x + CIRCLE_RADIUS) - paddle_x >= 0) && 
                    ball.y + CIRCLE_RADIUS >= PADDLE_POSITION_Y) {
    posX = (ball.x - (paddle_x + PADDLE_WIDTH/2)) / (PADDLE_WIDTH/2);
    
    ball.vel_x = speedXY * INFLUENCE_X * posX;

    ball.vel_y = sqrt(abs(speedXY*speedXY - ball.vel_x*ball.vel_x)) * (ball.vel_y > 0 ? -1 : 1);
  }

  /* Test for collisions with blocks */
  for(i = 0; i < NUM_BLOCKS_UD; i++) {
    for(j = 0; j < NUM_BLOCKS_LR; j++) {
      Block block = blocks[i][j];
      if (block.active == false)
        continue;
                
      /* Collision! */
      if (((ball.x - CIRCLE_RADIUS) - block.x <= BLOCK_WIDTH && (ball.x - CIRCLE_RADIUS) - block.x >= 0) && 
                    ((ball.y - CIRCLE_RADIUS) - block.y <= BLOCK_HEIGHT && (ball.y + CIRCLE_RADIUS) + (block.y - BLOCK_HEIGHT) >= 0)) {
          /* Collision on the side */
          if((ball.x - CIRCLE_RADIUS) - block.x <= BLOCK_WIDTH && (ball.x - CIRCLE_RADIUS) - block.x >= 0)
            ball.vel_x *= -1;
            
          /* Collision on top/bottom */
          if ((ball.y - CIRCLE_RADIUS) - block.y <= BLOCK_HEIGHT && (ball.y + CIRCLE_RADIUS) + (block.y - BLOCK_HEIGHT) >= 0)
            ball.vel_y *= -1;
          blocks[i][j].active = false;
      }
    }
  }
  
  ball.y += ball.vel_y;
  ball.x += ball.vel_x;

  /******** END OF GAME LOGIC ********/
  
  /* Draw the game */
  display.clearDisplay();
  
  for(i = 0; i < NUM_BLOCKS_UD; i++) {
    for(j = 0; j < NUM_BLOCKS_LR; j++) {
      if(blocks[i][j].active == true) {
        if(blocks[i][j].filled == false)
          drawRoundRect(blocks[i][j].x, blocks[i][j].y);
        else
          fillRoundRect(blocks[i][j].x, blocks[i][j].y);
      }
    }
  }
  
  fillRect(paddle_x, PADDLE_POSITION_Y);
  drawCircle((int)ball.x, (int)ball.y);
  
  display.display();

  delay(15);
}

bool leftButton(void) {
  return digitalRead(LEFT_BUTTON) == LOW;
}

bool rightButton(void) {
  return digitalRead(RIGHT_BUTTON) == LOW;
}

void fillRect(int x, int y) {
  display.fillRect(x, y, BLOCK_WIDTH, BLOCK_HEIGHT, 
        SSD1306_INVERSE);
}

void drawCircle(int x, int y) {
  display.drawCircle(x, y, CIRCLE_RADIUS, SSD1306_INVERSE);
}

void drawRoundRect(int x, int y) {
  display.drawRoundRect(x, y, BLOCK_WIDTH, BLOCK_HEIGHT,
      10/4, SSD1306_INVERSE);
}

void fillRoundRect(int x, int y) {
   display.fillRoundRect(x, y, BLOCK_WIDTH, BLOCK_HEIGHT,
      10/4, SSD1306_INVERSE);
}

void drawChar(char c) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  display.write(c);
}


void scrollText(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("Breakout"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}
