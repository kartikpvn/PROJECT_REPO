#include <GL/glut.h>///#include <glut.h>
#include <GL/gl.h>///#include <glut.h>
#include <GL/glu.h>///#include <glut.h>
#include <cmath>
#define FROM_RIGHT      1 
#define FROM_LEFT       2 
#define FROM_TOP        3 
#define FROM_BOTTOM     4 

int WINDOW_WIDTH ,WINDOW_HEIGHT;

int playerResult=0;
static float Xspeed=5,Yspeed=5; 
 static float delta=1; 
const float DEG2RAD = 3.14159/180;
char string [100];

class Rectangle{
        public:
        float left,top,right,bottom;
        Rectangle();
        void drawRectangle(); 
        Rectangle(int x, int y, int z , int w); 
};

Rectangle::Rectangle(int x, int y, int z , int w){left = x; top = y; right = z; bottom = w;
};     

Rectangle::Rectangle(){
  left = 0;
   top = 0;
   right = 0;
   bottom = 0;
};


class Circle{
    float radius;
    int c_x,c_y;
    public: 
     Circle(int r); 
	   void drawCircle();
};

Circle::Circle(int r){
  radius = r;
  c_x = 300;
  c_y = 300;
};

Rectangle ball(100,100,120,120);

Rectangle wall(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
Circle  circ_ball(5);

// Player boards of player_1 player_2
Rectangle player_1(0,490,40,510); 
Rectangle player_2(0,0,40,10);

void Rectangle::drawRectangle() 
{ 
        glBegin(GL_QUADS); 
        glVertex2f(left,bottom);      //Left - Bottom 
        glVertex2f(right,bottom); 
        glVertex2f(right,top); 
        glVertex2f(left,top); 
        glEnd(); 
}

void Circle::drawCircle()
{
   glBegin(GL_LINE_LOOP);
 
   for (int i=0; i<=1000; i++)
   {
      float degInRad = i*DEG2RAD;
      glVertex2f(c_x + cos(i*degInRad)*radius, c_y + sin(i*degInRad)*radius);
   }
 
   glEnd();
}


void Timer(int v) 
{ 
         /* ??? ???? ????? ?? ??  5 ?? ????? ?? ??????? */  
        ball.left  +=  0.5*Xspeed; 
        ball.right  += 0.5*Xspeed; 
        ball.top   += 0.5*Yspeed; 
        ball.bottom += 0.5*Yspeed; 
 
        glutTimerFunc(1,Timer,1); // ??? ????? ???? ??? ?? ??? ?????? ?????? ?????? 
}

void drawText(char*string,int x,int y)
{
	  char *c;
	  glPushMatrix();
	  glTranslatef(x, y,0);
	  glScalef(0.1,-0.1,1);
  
	  for (c=string; *c != '\0'; c++)
	  {
    		glutStrokeCharacter(GLUT_STROKE_ROMAN , *c);
	  }
	  glPopMatrix();

}
 
int Test_Ball_Wall(Rectangle  &ball , Rectangle &wall) 
{ 
        if(ball.right >=wall.right) 
        return FROM_RIGHT;  
        if(ball.left <=wall.left) 
        return FROM_LEFT;  
        if(ball.top <=wall.top) 
        return FROM_TOP;  
        if(ball.bottom >=wall.bottom) 
        return FROM_BOTTOM;  
 
        else return 0 ; 
}

bool Test_Ball_Player(Rectangle &ball,Rectangle &player) 
{ 
        
		if(ball.bottom >= player.top && ball.left>= player.left && ball.right <=player.right ) 
        {
			playerResult++;
		
            return true;  
		}
 
            return false;  
}
 
// Key Board Messages 
void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 27:
         exit(0);
         break;
   }
}

//Key Board Message 
void inputKey(int key, int x, int y) {

	switch (key) {
		case GLUT_KEY_LEFT : 

			break;

		case GLUT_KEY_RIGHT : 

			break;


		case GLUT_KEY_UP : 

			break ;


		case GLUT_KEY_DOWN : 

			break;


	}
}

//Key Board Message 
void upkey(int key, int x, int y) {

	switch (key)
	{
 

	}
}

static unsigned int mouse_x=0; 
void MouseMotion(int x,int y)
{
 
	mouse_x=x;
}




 
// OnWindowResize 
void reshape (int w, int h)
{
	WINDOW_WIDTH =w ;
	WINDOW_HEIGHT =h ;

   glViewport (0, 0, (GLsizei) w, (GLsizei) h);

   glMatrixMode (GL_PROJECTION);
   glLoadIdentity ();
   gluOrtho2D (0, w, h, 0);


   glMatrixMode (GL_MODELVIEW);
   glLoadIdentity ();
   
}
