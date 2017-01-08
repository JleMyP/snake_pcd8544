#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_PCD8544.h"


#define menu_n 3
#define buttons_n 5
#define btn_up 0
#define btn_down 1
#define btn_left 2
#define btn_right 3
#define btn_c 4

#define display_led 9

#define GAME 1
#define GAME_OVER 2
#define SETTINGS 3

#define max_len 50


// sclk din d/c cs rst
// sclk din d/c rst
// d/c cd rst
Adafruit_PCD8544 display(8, 7, 6, 4, 5);

static const byte PROGMEM  keksik[] = {
  B00000000, B00001110, B00000000, //1
  B00000000, B00011110, B00000000, //2
  B00000001, B11111101, B00000000, //3
  B00000110, B00111111, B00000000, //4
  B00011000, B10011111, B00000000, //5
  B00100000, B00001100, B10000000, //6
  B00100010, B00000000, B10000000, //7
  B01001000, B00000100, B01000000, //8
  B01000000, B01000000, B01000000, //9
  B10000000, B00000001, B00100000, //10
  B10000010, B00010000, B00100000, //11
  B10000000, B00000000, B00100000, //12
  B01100000, B01000011, B11000000, //13
  B00110000, B10100100, B10000000, //14
  B00101001, B00011000, B10000000, //15
  B00010110, B00000001, B00000000, //16
  B00010000, B00000001, B00000000, //17
  B00010000, B00000001, B00000000, //18
  B00001000, B00000010, B00000000, //19
  B00000111, B11111100, B00000000  //20
};

byte buttons[] = {15, 16, 17, 18, 19};
byte last_states_buttons[] = {0, 0, 0, 0, 0};
byte processed_buttons[] = {0, 0, 0, 0, 0};
byte display_brightness = 1;
byte body[max_len*2], body_n, food[2], dir, score;
byte location, menu_select, keksik_pos, keksik_speed = 1;
byte max_x, max_y, center_x, center_y, player_w = 5, player_speed = 5;


void setup(){
  /*OCR1A = 50000;
  TIMSK1 = _BV(OCIE1A);
  TCCR1A = 0;
  TCCR1B = _BV(CS11)|_BV(CS10)|_BV(WGM12);*/
  
  display.begin(60);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  //display.println("blablabla");
  //display.display();
  randomSeed(analogRead(0));
  for(byte i=0; i<buttons_n; i++) digitalWrite(buttons[i], 1);
  analogWrite(display_led, display_brightness*25);
  settings();
  pinMode(13, 1);
  digitalWrite(13, 1);
  delay(300);
  digitalWrite(13, 0);
}

void loop(){
  //buttons_callback();
  if(location==GAME) loop_game();
  else if(location==GAME_OVER) loop_game_over();
  else if(location==SETTINGS) loop_settings();
  //cli();
  for(byte i=0; i<buttons_n; i++) processed_buttons[i] = 1;
  //sei();
}

//void buttons_callback(){
ISR(TIMER1_COMPA_vect){
  for(byte i=0; i<buttons_n; i++){
    if(digitalRead(buttons[i]) == last_states_buttons[i]){
      last_states_buttons[i] = !last_states_buttons[i];
      processed_buttons[i] = 0;
    }
  }
}

void loop_game(){
  if(!processed_buttons[btn_up] && last_states_buttons[btn_up] && dir!=2) dir = 1;
  if(!processed_buttons[btn_down] && last_states_buttons[btn_down] && dir!=1) dir = 2;
  if(!processed_buttons[btn_left] && last_states_buttons[btn_left] && dir!=4) dir = 3;
  if(!processed_buttons[btn_right] && last_states_buttons[btn_right] && dir!= 3) dir = 4;
  if(!processed_buttons[btn_c] && last_states_buttons[btn_c]){
    settings();
    return;
  }
  move();
  draw_game();
  delay((10-player_speed)*10);
}

void loop_game_over(){
  draw_game_over();
  keksik_pos += keksik_speed;
  if(keksik_pos>=25 && keksik_speed>0 || keksik_pos<=5 && keksik_speed<0) keksik_speed = -keksik_speed;
  if(!processed_buttons[btn_c] && last_states_buttons[btn_c]){
    new_game();
  } else delay(20);
}

void loop_settings(){
  if(!processed_buttons[btn_up] && last_states_buttons[btn_up] && menu_select>1) menu_select--;
  if(!processed_buttons[btn_down] && last_states_buttons[btn_down] && menu_select<menu_n) menu_select++;
  if(!processed_buttons[btn_left] && last_states_buttons[btn_left]){
    if(menu_select==1 && player_w>1) player_w--;
    else if(menu_select==2 && player_speed > 1)player_speed--;
    else if(menu_select==3 && display_brightness > 0){
      display_brightness--;
      analogWrite(display_led, display_brightness*25);
    }
  }
  if(!processed_buttons[btn_right] && last_states_buttons[btn_right]){
    if(menu_select==1 && player_w < 10) player_w++;
    else if(menu_select==2 && player_speed < 10) player_speed++;
    else if(menu_select==3 && display_brightness < 10){
      display_brightness++;
      analogWrite(display_led, display_brightness*25);
    }
  }
  if(!processed_buttons[btn_c] && last_states_buttons[btn_c]){
    set_player_w();
    new_game();
  }else draw_settings();
}

void move(){
  byte head[2];
  if(dir==1 && body[1]-player_w > 9){
    head[0] = body[0];
    head[1] = body[1]-player_w;
  } else if(dir==2 && body[1]+player_w*2 < 47){
    head[0] = body[0];
    head[1] = body[1]+player_w;
  } else if(dir==3 && body[0]-player_w > 1){
    head[0] = body[0]-player_w;
    head[1] = body[1];
  } else if(dir==4 && body[0]+player_w*2 < 83){
    head[0] = body[0]+player_w;
    head[1] = body[1];
  } else return;
  
  if(check_in_body(head[0], head[1])){
    game_over();
    return;
  }
  if(head[0] == food[0] && head[1] == food[1]){
    put_food();
    score++;
    body_n++;
  }
  byte new_body[max_len*2] = {head[0], head[1]};
  for(byte i=0; i<body_n*2; i+=2){
    new_body[i+2] = body[i];
    new_body[i+3] = body[i+1];
  }
  byte *body = new_body;
}

void draw_game(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Score: "+(String)score);
  display.drawRect(0, 8, 84, 40, 1);
  for(byte i=2; i<body_n*2; i+=2){
    display.drawRect(body[i], body[i+1], player_w, player_w, 1);
  }
  display.fillRect(body[0], body[1], player_w, player_w, 1);
  display.fillRect(food[0], food[1], player_w, player_w, 1);
  display.display();
}

void draw_game_over(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("GAME\nOVER");
  display.setCursor(0, 36);
  display.setTextSize(1);
  display.print("Score: "+(String)score);
  display.drawBitmap(55, keksik_pos, keksik, 20, 20, 1);
  display.display();
}

void draw_settings(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("setting");
  display.setTextSize(1);
  if(menu_select==1) display.print(">");
  display.println("snake size: "+(String)player_w);
  if(menu_select==2) display.print(">");
  display.println("speed: "+(String)player_speed);
  if(menu_select==3) display.print(">");
  display.print("brightness: "+(String)display_brightness);
  display.display();
  digitalWrite(13, 1);
  delay(300);
  digitalWrite(13, 0);
  delay(50);
}

bool check_in_body(int x, int y){
  for(byte i=0; i<max_len*2; i+=2){
    if(body[i] == x && body[i+1] == y) return true;
  }
  return false;
}

void put_food(){
  byte x, y;
  while(true){
    x = random(2, max_x+1); x -= (x-2)%player_w;
    y = random(10, max_y+1); y -= (y-10)%player_w;
    if(!check_in_body(x, y)) break;
  }
  food[0] = x;
  food[1] = y;
}

void new_game(){
  location = GAME;
  dir = score = 0;
  body_n = 5;
  for(byte i=0; i<5; i++){
    body[i*2] = center_x+1+i*player_w;
    body[i*2+1] = center_y;
  }
  for(byte i=10; i<max_len*2; i++) body[i] = 0;
  put_food();
  draw_game();
  delay(250);
}

void game_over(){
  location = GAME_OVER;
  keksik_pos = 12;
}

void settings(){
  location = SETTINGS;
  menu_select = 1;
}

void set_player_w(){
  max_x = 1+80-80%player_w;
  max_y = 9+36-36%player_w;
  center_x = 2+82/3-(82/3)%player_w;
  center_y = 10+38/3-(38/3)%player_w;
}
