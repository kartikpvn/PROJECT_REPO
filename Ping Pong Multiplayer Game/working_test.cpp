#include "game_test.h"
#include "network.h"

#include <iostream>

int pcResult =0;
using namespace std;

void Render()
{
	glClear(GL_COLOR_BUFFER_BIT );
	glLoadIdentity();
 
//	sprintf(string,"PC : %d ",pcResult); // ÊÍæíá ÑÞã Çáì äÕ
	//drawText(string,10,80); // ØÈÇÚÉ ÇáäÕ
	//sprintf(string,"Player : %d ",playerResult);
	//drawText(string,10,120);

	wall.left=wall.top=0;
	wall.right=WINDOW_WIDTH;
	wall.bottom=WINDOW_HEIGHT;

  ball.drawRectangle();
  circ_ball.drawCircle();
  
	if(Test_Ball_Wall(ball,wall)== FROM_RIGHT) 
             Xspeed=-delta; 

    if(Test_Ball_Wall(ball,wall)== FROM_LEFT) 
    		Xspeed=delta; 

    if(Test_Ball_Wall(ball,wall)== FROM_TOP) 
    		Yspeed=delta; 

    if(Test_Ball_Wall(ball,wall)== FROM_BOTTOM) 
	{
		Yspeed=-delta; 
		pcResult +=1;
	}
 
	  player_1.drawRectangle();   
    player_1.left=mouse_x-20; 
    player_1.right=mouse_x+40; 
    // sending my data to the server
    udp_client_server::udp_client *client = new udp_client_server::udp_client("128.9.160.17",1000);
    char player_1_buff[4];
    snprintf(player_1_buff, sizeof(player_1_buff), "%d", mouse_x); 
    client->send(player_1_buff,4);
    
  // setting time out   
     struct timeval tv = {0,1000};
        
        if (setsockopt(client->get_socket(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            perror("Error");
        }


   // receiving data from server  
    char player_2_buff[4]; 
    player_2.drawRectangle();
    client->recv(player_2_buff, 4 );   
    unsigned int temp;
    sscanf(player_2_buff, "%u", &temp);

    player_2.left=temp-20; 
    player_2.right=temp+40; 
    delete client;


    if(Test_Ball_Player(ball,player_1)==true) 
            Yspeed=-delta; 

	glutSwapBuffers();
}


int main(int argc, char** argv)
{ glutInit(&argc, argv);

  glutInitDisplayMode ( GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize (750,500); 
  glutInitWindowPosition (0, 0);
  glutCreateWindow ("test ping pong game");
  glClearColor (1, 0, 1, 0);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  glutDisplayFunc(Render); 
  
   glutIdleFunc(Render);
   

   glutTimerFunc(1,Timer,1);

   glutReshapeFunc(reshape);
   
   glutKeyboardFunc(keyboard);

   //glutSpecialFunc(inputKey);
   //glutSpecialUpFunc(upkey);

   glutPassiveMotionFunc(MouseMotion);


   glutMainLoop();
   return 0;
}
