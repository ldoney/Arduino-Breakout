/*
 * Author: Lincoln Doney
 * Program: Arduino Breakout
 * Purpose: This is an Arduino version of breakout! There's capabilities for sounds and 
 *  for scores, although currently there is only one level. Have fun!
 * Components: This game requires an arduino with at least 21246 bytes of memory, I used
 *  an arduino uno R3 because of its large amount of memory.
 *  This game also requires 2 buttons for the left and right side, and an active buzzer
 *  if you want sound. Finally, it requires an adafruit SSD1306 OLED display for the visual
 *  component. When I get the chance I'll post the schematics in github.
 */
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
#define SPEAKER 8

#define NUM_BLOCKS_LR 5
#define NUM_BLOCKS_UD 3
#define BLOCK_PADDING 3
#define CIRCLE_RADIUS 3

#define PADDLE_POSITION_Y (display.height()-BLOCK_PADDING)

#define INFLUENCE_X 0.75
#define MAX_SPEED 3
#define WALL_TONE 400
#define HIT_TONE 600
#define PADDLE_TONE 800

#define BLOCK_WIDTH (display.width()/NUM_BLOCKS_LR - BLOCK_PADDING)
#define PADDLE_WIDTH BLOCK_WIDTH

#define BLOCK_HEIGHT ((display.height()/2)/NUM_BLOCKS_UD - BLOCK_PADDING)
#define PADDLE_HEIGHT BLOCK_HEIGHT

#define STATUS_CYCLES 5

/* Ball definition */
typedef struct {
  double x = 0;
  double y = 0;
  double vel_x = 0;
  double vel_y = 1.2;
} Ball;

int paddle_x;
Ball ball;
bool gameOver;
int score;

/* Block definition */
typedef struct {
  int x = 0;
  int y = 0;
  bool active = true;
  int hitcycles = -1;
  bool filled = true;
} Block;

Block blocks[NUM_BLOCKS_UD][NUM_BLOCKS_LR];

void setup() {
  /* For debugging (not really used */
  Serial.begin(9600);

  /* Try to start the display */
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Something broke!");
    /* Don't do anything */
    while(true){}
  }

  /* Initialize the buttons */
  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);

  /* Clear display (not really needed but eh) */
  display.clearDisplay();

  /* Set the default values */
  paddle_x = (display.width()/2) - (display.width()/NUM_BLOCKS_LR - BLOCK_PADDING)/2;
  ball.x = (double)((display.width()/2));
  ball.y = (double)(PADDLE_POSITION_Y - 6);

  /* Reset board */
  for(int i = 0; i < NUM_BLOCKS_UD; i++) {
    for(int j = 0; j < NUM_BLOCKS_LR; j++) {
      /* Set default values */
      blocks[i][j].active = true;
      blocks[i][j].x = j * (display.width()/NUM_BLOCKS_LR);
      blocks[i][j].y = i * ((display.height()/2)/NUM_BLOCKS_UD);
      blocks[i][j].filled = (i+j) % 2 == 0;
      blocks[i][j].hitcycles = -1;
    }
  }
  gameOver = true;

  /* Display title screen */
  display.clearDisplay();
  printText(0, 10, 2, "Breakout!");
  printText(0, 40, 1, "Press any button to  start!");
  display.display();

  /* Wait for user input */
  while(gameOver == true) {
    if(leftButton() || rightButton())
      gameOver = false;
    delay(15);
  }
  score = 0;
}

void loop() {
  int i, j;
  double speedXY, posX;

  /* Turn off the speaker if it's still going */
  noTone(SPEAKER);
  
  if(gameOver == false) {
    
    /******** GAME LOGIC ********/

    /* Left button action */
    if(leftButton()) {
      /* Remain within bounds */
      if(paddle_x > 0)
        paddle_x -= 5;
    } 
    if(rightButton()) {
      /* Remain within bounds */
      if(paddle_x + PADDLE_WIDTH < display.width())
        paddle_x += 5;
    }

    /* Keep the velocity within a certain maximum */
    ball.vel_y = abs(ball.vel_y) > MAX_SPEED ? 
        MAX_SPEED * (ball.vel_y > 0 ? -1 : 1) : 
        ball.vel_y;        
    ball.vel_x = abs(ball.vel_x) > MAX_SPEED ? 
        MAX_SPEED * (ball.vel_x > 0 ? -1 : 1) : 
        ball.vel_x;

    /* Define the speed for future calculations */
    speedXY = sqrt(ball.vel_x*ball.vel_x + ball.vel_y*ball.vel_y);
    
    /* Collision with wall! */
    if (ball.x - CIRCLE_RADIUS < 0 || ball.x + CIRCLE_RADIUS > display.width()) {
      /* Bounce! */
      ball.vel_x *= -1;
      /* Play a sound */
      tone(SPEAKER, WALL_TONE);
    }

    /* Collision with roof */
    if (ball.y - CIRCLE_RADIUS < 0) {
      /* Bounce! */
      ball.vel_y *= -1;
      /* Play a sound */
      tone(SPEAKER, WALL_TONE);
    }

    /* Game over! */
    if(ball.y + CIRCLE_RADIUS > display.height()) {
      /* End the game */
      gameOver = true;

      /* Print ending screen */
      display.clearDisplay();
      printText(0, 0, 2, "Game over!");
      printText(0, 40, 1, "Score: ");
      printInt(35, 40, 1, score);

      /* Play game over sound */
      display.display();
      for(i = 500; i >= 50; i--) {
        tone(SPEAKER, i);      
        delay(5);
      }
      
      /* Reset gameplay cycle */
      return;
    }

    /* Collision with paddle! */
    if (((ball.x - CIRCLE_RADIUS) - paddle_x <= PADDLE_WIDTH && (ball.x + CIRCLE_RADIUS) - paddle_x >= 0) && 
                      ball.y + CIRCLE_RADIUS >= PADDLE_POSITION_Y) {
      /* Compute new velocity */
      posX = (ball.x - (paddle_x + PADDLE_WIDTH/2)) / (PADDLE_WIDTH/2);
      ball.vel_x = speedXY * INFLUENCE_X * posX;
      ball.vel_y = sqrt(abs(speedXY*speedXY - ball.vel_x*ball.vel_x)) * (ball.vel_y > 0 ? -1 : 1);

      /* Play beep for hit! */
      tone(SPEAKER, PADDLE_TONE);
    }
  
    /* Test for collisions with blocks */
    for(i = 0; i < NUM_BLOCKS_UD; i++) {
      for(j = 0; j < NUM_BLOCKS_LR; j++) {
        Block block = blocks[i][j];
        if (block.active == false)
          continue;
                  
        /* Collision! */
        if (((ball.x - CIRCLE_RADIUS) - (block.x + BLOCK_WIDTH) <= 0 && (ball.x + CIRCLE_RADIUS) - block.x >= 0) && 
                      ((ball.y - CIRCLE_RADIUS) - (block.y + BLOCK_HEIGHT) <= 0 && (ball.y + CIRCLE_RADIUS) - block.y >= 0)) {
                        
            /* Collision on the side */
            if((ball.x - CIRCLE_RADIUS) - (block.x + BLOCK_WIDTH) <= 0 && (ball.x + CIRCLE_RADIUS) - block.x >= 0)
              ball.vel_x *= -1.1;
              
            /* Collision on top/bottom */
            if ((ball.y - CIRCLE_RADIUS) - (block.y + BLOCK_HEIGHT) <= 0 && (ball.y + CIRCLE_RADIUS) - block.y >= 0)
              ball.vel_y *= -1.1;
              
            blocks[i][j].active = false;
            blocks[i][j].hitcycles = STATUS_CYCLES;
            tone(SPEAKER, HIT_TONE);
            score += 100;
            
            /* You can only hit 1 at a time */
            continue;
        }
      }
    }

    /* Move ball */
    ball.y += ball.vel_y;
    ball.x += ball.vel_x;

    /* Test to see if all the blocks are gone */
    bool didntwin = false;
    for(i = 0; i < NUM_BLOCKS_UD; i++) {
      for(j = 0; j < NUM_BLOCKS_LR; j++) {
        if(blocks[i][j].active == true)
          didntwin = true;
      }
    }

    /* Game over! They won! */
    if(didntwin == false) {
      gameOver = true;

      /* Print win screen */
      display.clearDisplay();
      printText(0, 0, 2, "You win!");
      printText(0, 40, 1, "Score: ");
      printInt(35, 40, 1, score);
      display.display();

      /* Play game won sound */
      for(i = 50; i < 1000; i+=2) {
        tone(SPEAKER, i);      
        delay(5);
      }
      return;
    }
    
    /******** END OF GAME LOGIC ********/
    
    /* Draw the game */
    display.clearDisplay();
    
    for(i = 0; i < NUM_BLOCKS_UD; i++) {
      for(j = 0; j < NUM_BLOCKS_LR; j++) {
        if(blocks[i][j].active == true) {
          /* Make a checkered pattern bc why not */
          if(blocks[i][j].filled == false)
            drawRoundRect(blocks[i][j].x, blocks[i][j].y);
          else
            fillRoundRect(blocks[i][j].x, blocks[i][j].y);
        }
        /* Print out that there was 100 points scored */
        if(blocks[i][j].hitcycles > 0) {
            printText(blocks[i][j].x, blocks[i][j].y, 1, "100");
            blocks[i][j].hitcycles--;
        }
      }
    }

    /* Print the ball, score, and the paddle */
    printInt(0, display.height() - 10, 1, score);
    fillRect(paddle_x, PADDLE_POSITION_Y);
    drawCircle((int)ball.x, (int)ball.y);
    display.display();
  }

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
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.cp437(true);

  display.write(c);
}

void printText(int x, int y, int siz, char *text) {
  display.setTextSize(siz);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.println(text);
}

void printInt(int x, int y, int siz, int val) {
  display.setTextSize(siz);
  display.setTextColor(SSD1306_INVERSE);
  display.setCursor(x, y);
  display.println(val);
}
