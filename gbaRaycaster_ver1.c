/*
--------------------------------------------------------------------
@author: jduranmaster
@title: jduranWolf.c
@versión: 1.0
********************************************************************
Parte de este proyecto está basado en el código original de JESPA_3D
Gracias al autor por subir el trig.h y compartir sus conocimientos 
en el GBADEV.
********************************************************************
--------------------------------------------------------------------
*/

//----------------------------------------------------------------
// ficheros de cabecera incluidos. 
//----------------------------------------------------------------
#include "gba.h"		//definición de los registros de la GBA.
#include "keypad.h"		//definición del PAD
#include "screenmode.h"	//modos de pantalla --> basado en el tuto de Loriak
#include "trig.h"		//trig lookup tables --> tomado del ejemplo de JESPA_3D


//----------------------------------------------------------------
// constantes del programa.
//----------------------------------------------------------------
#define SCREENWIDTH				240 //ancho de nuestra pantalla.
#define SCREENHEIGHT			160 //alto de nuestra pantalla.
#define GRIDWIDTH				32
#define GRIDHEIGHT				32
#define PLAY_LENGTH				(207.85)
#define W						1   // MURO.
#define O						0   // ESPACIO ABIERTO.
#define PI						(3.14159) //el número PI.
#define SHORT_LINE				1200	// screenwidth/2 * 10
#define LONG_LINE				1800	// screenwidth/2 * 15
#define true					1
#define false					0

const u16 gPalette[] = { // 0..16                     			
	0x7203,	0x7205,	0x7208,	0x720A,	0x720D,	0x720F,	0x7212,	0x7214,
	0x2945,	0x2946,	0x2948,	0x294A,	0x294C,	0x294E,	0x6B5A,	0x56B5,};

//--------------------------------------------------------------------------------------------------------------------------
// Este es el mapa usado. Lo podemos cambiar tranquilamente para crear nuestros propios escenarios. Como se puede ver, 
// W significa "MURO" (#define W 1) y O significa "ESPACIO ABIERTO" (#define O 0 ). El Mapa es un cuadrado de 16x16.
//--------------------------------------------------------------------------------------------------------------------------
#define MAPWIDTH				16
#define MAPHEIGHT				16

const char fMap[]=
{//     0         5         10        15
		W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W, // 0
        W,O,O,O,O,O,O,O,O,O,O,O,O,O,O,W,
        W,O,O,O,O,O,O,O,O,O,O,O,O,O,O,W,
        W,O,O,O,O,O,O,O,O,O,O,W,O,O,O,W,
        W,O,O,O,O,O,W,O,W,O,O,W,O,O,O,W,
        W,O,O,O,O,O,W,O,W,W,O,W,O,O,O,W, // 5
        W,O,O,O,O,O,W,O,W,O,O,W,O,O,O,W,
        W,O,O,O,O,O,W,W,W,O,O,W,O,O,O,W,
        W,O,O,O,O,O,O,W,O,O,O,W,O,O,O,W,
        W,O,O,O,O,O,O,W,W,W,W,W,O,O,O,W,
        W,O,W,O,O,W,O,O,O,O,O,O,O,O,O,W, // 10
        W,O,W,O,O,W,O,O,O,O,O,O,O,O,O,W,
        W,O,W,O,O,W,O,O,O,O,O,O,O,O,O,W,
        W,O,O,W,W,O,O,O,O,O,O,O,O,O,O,W,
        W,O,O,O,O,O,O,O,O,O,O,O,O,O,O,W,
        W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W, // 15
};

//----------------------------------------------------------------
// variables globales.
//----------------------------------------------------------------

u32 nPlayerX;
u32 nPlayerY;
u32 nPlayerAngle;

int fKeyUp;		//bool fKeyUp=false;
int fKeyDown;	//bool fKeyDown=false;
int fKeyLeft;	//bool fKeyLeft=false;
int fKeyRight;	//bool fKeyRight=false;

u16* FrontBuffer = (u16*)0x6000000;
u16* BackBuffer = (u16*)0x600A000;
u16* videoBuffer;
u16* paletteMem = (u16*)0x5000000;

//----------------------------------------------------------------
// prototipo de la funciones utilizadas. 
//----------------------------------------------------------------
void drawBackground(void);		// esta función básicamente permite limpiar la pantalla de la GBA.
void renderWalls();				// permite dibujar los muros.
void WaitForVsync(void);		// función de espera al vsync.
void updateKeyVars();			// comprueba el registro de teclado y modifica las cuatro variables globales definidas antes.
void Flip(void);				// conmuta al buffer que va ser visto por la pantalla de la GBA.
void move(int moveup);			// nos permite mover al personaje. También gestiona el tema de las colisiones con los muros.
s32 absint(s32 d);				// devuelve el valor absoluto de un valor entero.
s32 distAngle(s32 s1, s32 s2);  // devuelve la distancia entre dos angulos. 

//----------------------------------------------------------------
// PROGRAMA PRINCIPAL . MAIN(*).
//----------------------------------------------------------------

int main(int argc, char* argv[]){
	char x; 
	char changed = 0;

	SetMode(MODE_4 | BG2_ENABLE ); //en este ejemplo vamos a utilizar el modo gráfico 4 de la GBA y además vamos a activar
	//el background MODE 2.

	for(x = 0; x < 16; x++)	// plaeta
		paletteMem[x] = gPalette[x];	

	nPlayerX = 96;		// coordenadas iniciales X
	nPlayerY = 128;		// coordenadas iniciales X
	nPlayerAngle = 0;	// hacia la derecha.

	Flip();// conmuta al buffer que va ser visto por la pantalla de la GBA.
	drawBackground();
	renderWalls(); // dibujamos los muros.
	Flip();// conmuta al buffer que va ser visto por la pantalla de la GBA.

	while(true){// bucle infinito.
		// en el bucle infinito procesamos los movimientos del personaje.
		if (fKeyLeft){
			nPlayerAngle += 10;
			if (nPlayerAngle >= 360) nPlayerAngle = 0;
			changed = 1;
		}

		else if (fKeyRight){
			if (nPlayerAngle < 10) nPlayerAngle = 350;
			else nPlayerAngle -= 10;
			changed = 1;
		}

		if (fKeyUp){
			move(1); // comprueba si ha habido colisiones del personaje con
			//alguno de los muros después de haberse producido un movimiento.
			changed = 1;
		}
		else if (fKeyDown){
			move(0); // comprueba si ha habido colisiones del personaje con
			//alguno de los muros después de haberse producido un movimiento.
			changed = 1;
		}


		if (changed) // only redraw if they push a key and change the screen
		{
			changed = 0;
			drawBackground(); // esta función básicamente permite limpiar la pantalla de la GBA.
			renderWalls(); // permite dibujar los muros.
			WaitForVsync(); // función de espera al vsync.
			Flip(); // conmuta al buffer que va ser visto por la pantalla de la GBA.
		}
		updateKeyVars();
	}

	return 0;
}//fin del metodo.

//------------------------------------------------------------------------------------------
// nos permite mover al personaje. También gestiona el tema de las colisiones con los muros.
//------------------------------------------------------------------------------------------
void move(int moveup) {
	double deltaX;
	double deltaY;
	
	if (moveup){
		deltaX = (tableCOS[nPlayerAngle] * 15);
		deltaY = -1*(tableSIN[nPlayerAngle] * 15);
	}
	else{
		deltaX = -1*(tableCOS[nPlayerAngle] * 15);
		deltaY = (tableSIN[nPlayerAngle] * 15);
	}
	
	int newX = (int)(nPlayerX + deltaX + (8 * abs(deltaX) / deltaX));
	int newY = (int)(nPlayerY + deltaY + (8 * abs(deltaY) / deltaY));
	
	if (fMap[(newX/64) + MAPWIDTH * (newY/64)] == O) {
		nPlayerX += (int) deltaX;
		nPlayerY += (int) deltaY;
	} else if (fMap[(newX/64) + MAPWIDTH * (nPlayerY/64)] == O) {
		nPlayerX += (int) deltaX;
	} else if (fMap[(nPlayerX/64) + MAPWIDTH * (newY/64)] == O) {
		nPlayerY += (int) deltaY;
	}
}//fin de la función.

//----------------------------
// función de espera al vsync.
//----------------------------
void WaitForVsync(void){
	while(REG_VCOUNT<160);
}//fin de la función.

//------------------------------------------------------------------------------------------------------------------
//esta función nos permite ir chequeando las variables que hemos definido para el control de las teclas de dirección.
//la correspondencia con las teclas del PAD está registrada en el fichero de cabecera keypad.h.
//------------------------------------------------------------------------------------------------------------------
void updateKeyVars(){
	fKeyUp = fKeyDown = fKeyLeft = fKeyRight = 0;

	if(!((*KEYS) & KEY_UP)){
		fKeyUp = 1;
	}
	if(!((*KEYS) & KEY_DOWN)){
		fKeyDown = 1;
	}
	if(!((*KEYS) & KEY_LEFT)){
		fKeyLeft=1;
	}
	if(!((*KEYS) & KEY_RIGHT)){
		fKeyRight=1;
	}
	if(!((*KEYS) & KEY_A)){
		//esta tecla no tiene función asignada.... aun.
	}
	if(!((*KEYS) & KEY_B)){
		//esta tecla no tiene función asignada.... aun.
	}
	if(!((*KEYS) & KEY_R)){
		//esta tecla no tiene función asignada.... aun.
	}
	if(!((*KEYS) & KEY_L)){
		//esta tecla no tiene función asignada.... aun.
	}
}//fin de la función.

//----------------------------------------------------------------
// esta función básicamente permite limpiar la pantalla de la GBA.
//----------------------------------------------------------------
void drawBackground(void){
	u16* destBuffer =videoBuffer;
	u8 loop;
	for (loop = 0; loop < 8; loop++)
	{
		u16 val = (loop << 8) + loop;
		u16* finalAdr=destBuffer+SHORT_LINE;
		while(destBuffer<finalAdr)
			*destBuffer++=val;
	}
	for (loop = 8; loop < 14; loop++)
	{
		u16 val = (loop<<8) + loop;
		u16* finalAdr=destBuffer+LONG_LINE;
		while(destBuffer<finalAdr)
			*destBuffer++=val;
	}
}//fin de la función.


//-----------------------------------------------
// devuelve el valor absoluto de un valor entero.
//-----------------------------------------------
s32 absint(s32 d){
	if (d < 0) d *= -1;
	return d;
}//fin de la función.

//-----------------------------------------
// devuelve la distancia entre dos angulos.
//----------------------------------------- 
s32 distAngle(s32 s1, s32 s2){
	s32 temp;
	if (s2 >= s1)
		temp = s2 - s1;
	else
		temp = s1 - s2;
	if (temp > 30) temp -= 360;
	return absint(temp);
}//fin de la función.

//-------------------------------------------------------------------------------------------------------
// Aqui dejo la definición del sistema de coordenadas que emplea el autor para definir la tabla en trig.h
//-------------------------------------------------------------------------------------------------------
//                       90 deg, y--
//                           |
//                           |
//                 QUAD2     |     QUAD1
//                           |
//                           |
//                           |
// x--, 180 deg --------------------------- 0 deg, x++
//                           |
//                           |
//                 QUAD3     |     QUAD4
//                           |
//                           |
//                           |
//                      270 deg, y++
//----------------------------------------------------

//------------------------------------------------------------------------------
// Esta función nos permite pintar los muros que forman parte del mapa definido.
//------------------------------------------------------------------------------
void renderWalls(){
	u32		loop;		
	s16		curAngle;
	s32		gridX;		// coordenada X en el mapa
	s32		gridY;		// coordenada Y en el MAPA
	u16*	destBuffer = videoBuffer; // apunta al buffer donde se pintan los muros.
	u8		x,y;		
	
	double	horzLength; // distancia al muro en horizontal (desde el pto de perpectiva del tio que anda.)
	double	vertLength; // distancia al muro en vertical (desde el pto de perpectiva del tio que anda.)
	double*	minLength;	
	u32		lineLength;	

	char darkgray = 0;	// vertical --> darkgray, horzontal --> lightgray

	double	fdistNextX;
	double	fdistNextY;
	int		ndistNextX; 
	int		ndistNextY;

	int		horzY;
	double  horzX;
	int		vertX;
	double	vertY; 

	curAngle = nPlayerAngle + 30;		// angulo de inicio.
	if (curAngle >= 360) curAngle -= 360;

	// 4 = SCREENWIDTH / 64 (TILEHEIGHT)
	for (loop = 0; loop < SCREENWIDTH; loop+=4) {
		// calcula la distancia horizontal.
		if (curAngle == 0 || curAngle == 180){
			// no hay un muro en la dirección horizontal
			horzLength = 9999999.00;
		}
		else{
			if (curAngle < 180){
				horzY = (nPlayerY/64) * 64; 
				ndistNextY = -64;
				double amountChange = ((s32) (horzY - nPlayerY) ) * tableINVTAN[curAngle];
				if (curAngle < 90 || curAngle > 270){
					if (amountChange < 0) amountChange *= -1;
				}
				else {
					if (amountChange > 0) amountChange *= -1;
				}
				horzX = nPlayerX + amountChange; 
				horzY--;
			}
			else {
				horzY = (nPlayerY/64) * 64 + 64; 
				ndistNextY = 64; 
				double amountChange = ((s32)(horzY - nPlayerY)) * tableINVTAN[curAngle];
				if (curAngle < 90 || curAngle > 270){
					if (amountChange < 0) amountChange *= -1; // should be pos
				}
				else {
					if (amountChange > 0) amountChange *= -1;
				}
				horzX = nPlayerX + amountChange;
			}
			fdistNextX = (64.00 * tableINVTAN[curAngle]);
			if ( (curAngle < 90) || (curAngle>270) ){
				if (fdistNextX < 0) fdistNextX *= -1;		// distancia positiva al siguiente bloque
			}
			else{
				if (fdistNextX > 0) fdistNextX *= -1;		// distancia negativa al siguiente bloque
			}

			while (true){
				gridX = (s32)(horzX / 64);
				gridY = (s32)(horzY / 64);
				if (gridX >= MAPWIDTH || gridY >= MAPHEIGHT || gridX < 0 || gridY < 0)
				{
					horzLength = 9999999.00;
					break;
				}
				else if (fMap[gridX+gridY*MAPHEIGHT])
				{
					horzLength = (horzX - nPlayerX) * tableINVCOS[curAngle];
					break;
				}
				horzX += fdistNextX;
				horzY += ndistNextY;
			}
		}
		// calcula la distancia vertical.
		if (curAngle == 90 || curAngle == 270){
			vertLength = 9999999.00;
		}
		else{
			if (curAngle < 90 || curAngle > 270){
				vertX = (nPlayerX/64) * 64 + 64;
				ndistNextX = 64;
				double amountChange = tableTAN[curAngle]*((s32)(vertX-nPlayerX));
				if (curAngle < 180){
					if (amountChange > 0) amountChange *= -1;
				}
				else
				{
					if (amountChange < 0) amountChange *= -1;
				}
				vertY = nPlayerY + amountChange; 
			}
			else{
				vertX = (nPlayerX/64) * 64; 
				ndistNextX = -64;			
				double amountChange = tableTAN[curAngle]*((s32)(vertX-nPlayerX));
				if (curAngle < 180){
					if (amountChange > 0) amountChange *= -1;
				}
				else{
					if (amountChange < 0) amountChange *= -1;
				}
				vertY = nPlayerY + amountChange; 
				vertX--;
			}
			fdistNextY = 64.00 * tableTAN[curAngle]; 
			if (curAngle < 180) {
				if (fdistNextY > 0) fdistNextY *= -1;
			}
			else{
				if (fdistNextY < 0) fdistNextY *= -1;
			}

			while (true){
				gridX = (s32)(vertX / 64);
				gridY = (s32)(vertY / 64);
				if (gridX >= MAPWIDTH || gridY >= MAPHEIGHT || gridX < 0 || gridY < 0)
				{
					vertLength = 9999999.00;
					break;
				}
				else if (fMap[gridX+gridY*MAPHEIGHT])
				{
					vertLength = (vertY - nPlayerY) * tableINVSIN[curAngle];
					break;
				}
				vertX += ndistNextX;
				vertY += fdistNextY;
			}
		}

		if (vertLength < 0) vertLength *= -1; 
		if (horzLength < 0) horzLength *= -1;

		if (vertLength < horzLength){
			minLength = &vertLength;
			darkgray = 1;
		}
		else{
			darkgray = 0;
			minLength = &horzLength;
		}

		//arreglar la distorsión.
		(*minLength) = (*minLength) * tableCOS[distAngle(curAngle, nPlayerAngle)];

		lineLength = absint((s32)((64.00 / *minLength) * PLAY_LENGTH)   );

		int end = (80 - lineLength/2);
		int start;
		if (end < 0){
			end = 160;
			start = 0;
		}
		else{
			start = end;
			end += lineLength;
		}

		u32 where = loop/2 + start*120;
		if (darkgray){
			for(y = start; y<end; y++)
			{
					destBuffer[where] = 0x0f0f;
					destBuffer[where+1] = 0x0f0f;
					where += 120;
			}
		}
		else{
			for(y = start; y<end; y++){
					destBuffer[where] = 0x0e0e;
					destBuffer[where+1] = 0x0e0e;
					where += 120;
			}
		}

		curAngle -= 1;
		if (curAngle < 0) curAngle += 360;

	}
}//fin de la función.

//--------------------------------------------------------------
// conmuta al buffer que va ser visto por la pantalla de la GBA.
//--------------------------------------------------------------
void Flip(void){
	if(REG_DISPCNT & BACKBUFFER){ 
		REG_DISPCNT &= ~BACKBUFFER; //el buffer activo es el frontal, por tanto limpiamos el buffer trasero.
		videoBuffer = BackBuffer; //apuntamos el buffer de dibujo al buffer trasero.
    }
    else{ 
		REG_DISPCNT |= BACKBUFFER; //el buffer activo es el trasero, por tanto limpiamos el buffer frontal.
		videoBuffer = FrontBuffer; //apuntamos el buffer de dibujo al buffer frontal.
	}
}//fin de la función.
