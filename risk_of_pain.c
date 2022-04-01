//Risk of Pain by Gab
#include "include/raylib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

//16:9 Screen Aspect Ratio and 60 fps
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TARGET_FPS 60

//Total number of hitboxes for a given screen
#define platform_1_count 10     //Screen 0
#define platform_2_count 8      //Screen 1
#define platform_3_count 5      //Screen 2
#define platform_4_count 6      //Screen 3
#define platform_5_count 6      //Screen 4

//Player stats (this can break the game, change wisely ツ)
#define BASE_LUCK 1             //Determines coin drop rate
#define BASE_SPEED 5            //Determines player x speed
#define BASE_DAMAGE 10          //Determines base damage dealt and taken
#define BASE_HEALTH_REGEN 0.03  //Determines base health regen (per frame)

//Gameplay stats (this can also break the game, change wisely ツ)
#define ENEMY_SPAWN_INTERVAL 1  //Time (in seconds) between enemy spawns, changing this to zero makes every enemy spawn at once
#define MAX_ENEMIES 10          //Max enemies on screen
#define MAX_COINS 256           //Max coins on screen (will cap at this number even if luck is too high)
#define MAX_CHESTS 3            //Max chests on screen

//Entity data (used for player/enemies/chests/coins)
typedef struct entityData {
    int type;
    float x;
    float y;
    float width;
    float height;
    float speed_x;
    float speed_y;
    float health;
    float maxHealth;
    float hitboxStart;
    float hitboxEnd;
    int sprite;
    int spawnTime;
    int price;
    int item;
} entityData;

//Game settings, shared between screens and the save games
typedef struct sharedOptions {
    float musicVolume;      //Music volume
    float sfxVolume;        //SFX volume
    int currentSaveSlot;    //Current save slot (1-3)
    int highscore;          //Player highscore, updated when going back to title screen or when dead
} sharedOptions;

//Player stats, used while in game
typedef struct playerStats {
    float baseLuck;
    float baseSpeed;
    float baseDamage;
    float baseHealthRegen;
    float health;
    float maxHealth;
    float healthAnim;
    float xp;
    float maxXp;
    int level;
    int gold;
    int totalGold;
    int enemiesKilled;
    int elapsedTime;
    int CooldownSkill2;
    int CooldownSkill3;
    int itemEquipped;
    int itemActive;
    int itemTimer;
} playerStats;

//Variables to be used with the overlay functions and the CRT effect
typedef struct overlayVariables {
    bool drawing;
    char infoToDraw[100];
    int counter;
    Texture2D crtBorder;
    int scanlineTimer;
    int scanlineStart;
    bool scanlineActive;
} overlayVariables;

//Custom function to overlay and animate any info on the screen automatically
void drawOverlayInfo(Font pixellariFont, overlayVariables *overlayData) {
    //Draw overlay text (if there's any)
    if (overlayData->drawing == true) {
        if (overlayData->counter < 90) {        //90 frames = 1.5 seconds
            DrawTextEx(pixellariFont, overlayData->infoToDraw, (Vector2){100+3, 620+3-overlayData->counter}, 20, 0, (Color){0, 0, 0, 127 - overlayData->counter*1.4}); //Drop shadow
            DrawTextEx(pixellariFont, overlayData->infoToDraw, (Vector2){100, 620-overlayData->counter}, 20, 0, (Color){253, 249, 0, 255 - overlayData->counter*2.8});  
            overlayData->counter++;    
        } else {
            overlayData->drawing = false;       //Resets counter when finished
            overlayData->counter = 0;
        }
    }    
    //Draw CRT effect (if active)
    //Timers used with the effect
    overlayData->scanlineTimer += 1;
    if (overlayData->scanlineTimer == 5)
        overlayData->scanlineStart += 1, overlayData->scanlineTimer = 0;
    if (overlayData->scanlineStart == 6)
        overlayData->scanlineStart = 0;
    //Toggle effect on/off
    if (IsKeyPressed(KEY_U))
        overlayData->scanlineActive = !overlayData->scanlineActive;
    //Draw CRT effect
    if (overlayData->scanlineActive == true) {
        for (int i = overlayData->scanlineStart; i <= 720; i += 6) {
            DrawLine(0, i, 1280, i, (Color){30, 30, 0, 70});
            DrawLine(0, i + 1, 1280, i + 1, (Color){30, 30, 0, 70});
            DrawLine(0, i + 2, 1280, i + 2, (Color){30, 30, 0, 70});
        }
        DrawTexture(overlayData->crtBorder, 0, 0, WHITE);
    }
}

//Custom function to send info to the drawOverlayInfo function
void sendOverlayInfo(char *info, overlayVariables *overlayData) {  
    //Saves new info in the overlayData struct
    overlayData->drawing = true;
    strcpy(overlayData->infoToDraw, info);
    overlayData->counter = 0;
}

//Custom function that creates a new save file with default settings
void createNewSave(int saveID, bool overwrite, overlayVariables *overlayData) {
    //Default save settings
    sharedOptions newSaveData;
    newSaveData.musicVolume = 60;
    newSaveData.sfxVolume = 50;
    newSaveData.currentSaveSlot = saveID;
    newSaveData.highscore = 0;
    //Prepare filename 
    FILE *save;
    char filename[100], id[2];
    strcpy(filename, "saves/save_slot_");   //Inital filename
    sprintf(id, "%d", saveID);
    strcat(filename, id);                   //Save ID
    strcat(filename, ".bin");               //File extension
    //Creates new save game with default values                                       
    save = fopen(filename, "rb");           //Check if save game already exists
    if (save == NULL || overwrite == true) {
        save = fopen(filename, "wb");       //If not, creates it (or overwrites it if the flag "overwrite" is set)
        fwrite(&newSaveData, 1, sizeof(sharedOptions), save); 
    }
    fclose(save);
}

//Custom function that loads current save data (transfers save info to current game settings)
void readSave(int saveID, sharedOptions *gameOptions, overlayVariables *overlayData) {
    //Prepare filename 
    FILE *save;
    char filename[100], id[2], overlay[100];
    strcpy(filename, "saves/save_slot_");   //Inital filename
    sprintf(id, "%d", saveID);
    strcat(filename, id);                   //Save ID
    strcat(filename, ".bin");               //File extension
    //Load save data into current game settings       
    save = fopen(filename, "rb");                               
    fread(gameOptions, 1, sizeof(sharedOptions), save); 
    fclose(save);
    //Send info to overlay on the screen
    strcpy(overlay, "Save ");
    strcat(overlay, id);
    strcat(overlay, " loaded");
    sendOverlayInfo(overlay, overlayData);
}

//Custom function that saves current game data (transfers current game settings to save info)
void writeSave(int saveID, sharedOptions *gameOptions) {
    //Prepare filename 
    FILE *save;
    char filename[100], id[2];
    strcpy(filename, "saves/save_slot_");   //Inital filename
    sprintf(id, "%d", saveID);
    strcat(filename, id);                   //Save ID
    strcat(filename, ".bin");               //File extension
    //Loads save data into current game settings       
    save = fopen(filename, "wb");                               
    fwrite(gameOptions, 1, sizeof(sharedOptions), save); 
    fclose(save);
}

//Custom function to check if the mouse is hovering over a centralized text
bool checkMouseHoverTextCenter(Font font, const char *text, Vector2 position, float fontSize, float spacing) {
    Vector2 currentMousePosition = GetMousePosition();
    Vector2 fontDimension = MeasureTextEx(font, text, fontSize, spacing);
    if (currentMousePosition.x >= position.x - fontDimension.x/2 && currentMousePosition.x <= position.x + fontDimension.x/2) 
        if (currentMousePosition.y >= position.y - fontDimension.y/2 && currentMousePosition.y <= position.y + fontDimension.y/2)
            return true;
    return false;
}

//Custom function to draw texts centralized. Changes color of the text if the mouse cursor is hovering it, returns true if MOUSE_LEFT_CLICK was pressed
bool DrawTextExCenterHover(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint, Color tint2) {
    Vector2 fontDimension = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 newPosition;
    newPosition.x = position.x - fontDimension.x/2;
    newPosition.y = position.y - fontDimension.y/2;
    DrawTextEx(font, text, (Vector2){newPosition.x + 3, newPosition.y + 3}, fontSize, spacing, (Color){0, 0, 0, tint.a/2}); //Drop Shadow
    //Check mouse hover
    if (checkMouseHoverTextCenter(font, text, position, fontSize, spacing)) //Changes color of the text if mouse is hovering it
        tint = tint2;  
    DrawTextEx(font, text, (Vector2){newPosition.x, newPosition.y}, fontSize, spacing, tint); //Text
    if (checkMouseHoverTextCenter(font, text, position, fontSize, spacing) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return true;    //If hovering and pressed, returns true
    return false;
}

//Custom function to draw the player highscore when needed (it was ineffective to copy/paste this everytime ツ)
void drawHighscore(sharedOptions *gameOptions, Font pixellariFont) {
    char highscore[6], currentHighscore[30];              //Generates a string with the highscore
    strcpy(currentHighscore, "Current Highscore: ");        
    sprintf(highscore, "%d", gameOptions->highscore);
    strcat(currentHighscore, highscore);
    DrawTextExCenterHover(pixellariFont, currentHighscore, (Vector2){SCREEN_WIDTH/2, 0.95*SCREEN_HEIGHT}, 15, 0, WHITE, WHITE);
}

//Custom function to check if the mouse is hovering over a rectangle (top-left drawing)
bool checkMouseHoverRectangle(Rectangle rec) {
    Vector2 currentMousePosition = GetMousePosition();   //Check if mouse is hovering the rectangle area     
    if (currentMousePosition.x >= rec.x && currentMousePosition.x <= rec.x + rec.width)        
        if (currentMousePosition.y >= rec.y && currentMousePosition.y <= rec.y + rec.height)
            return true;
    return false;
}

//Custom function to check collision between two rectangles
bool checkCollisionRectanglesLeft(Rectangle rec1, Rectangle rec2) {
    //Rec1 left side collides
    if (rec1.x > rec2.x && rec1.x < rec2.x + rec2.width) 
        if ((rec2.y > rec1.y && rec2.y < rec1.y + rec1.height) || (rec2.y + rec2.height > rec1.y && rec2.y + rec2.height < rec1.y + rec1.height) || (rec2.y <= rec1.y && rec2.y + rec2.height >= rec1.y + rec1.height))
            return true;
    return false;
}

//Custom function to check collision between two rectangles
bool checkCollisionRectanglesRight(Rectangle rec1, Rectangle rec2) {
    //Rec1 right side collides
    if (rec1.x + rec1.width > rec2.x && rec1.x + rec1.width < rec2.x + rec2.width) 
        if ((rec2.y > rec1.y && rec2.y < rec1.y + rec1.height) || (rec2.y + rec2.height > rec1.y && rec2.y + rec2.height < rec1.y + rec1.height) || (rec2.y <= rec1.y && rec2.y + rec2.height >= rec1.y + rec1.height))
            return true;
    return false;
}

//Custom function to check collision between two rectangles
bool checkCollisionRectanglesBottom(Rectangle rec1, Rectangle rec2) {
    //Rec1 bottom side collides
    if (rec1.y + rec1.height > rec2.y && rec1.y + rec1.height < rec2.y + rec2.height) 
        if ((rec1.x > rec2.x && rec1.x < rec2.x + rec2.width) || (rec1.x + rec1.width > rec2.x && rec1.x + rec1.width < rec2.x + rec2.width) || (rec1.x <= rec2.x && rec1.x + rec1.width >= rec2.x + rec2.width))
            return true;
    return false;
}

//Custom function to check collision between two rectangles
bool checkCollisionRectanglesTop(Rectangle rec1, Rectangle rec2) {
    //Rec1 top side collides
    if (rec1.y > rec2.y && rec1.y < rec2.y + rec2.height) 
        if ((rec1.x > rec2.x && rec1.x < rec2.x + rec2.width) || (rec1.x + rec1.width > rec2.x && rec1.x + rec1.width < rec2.x + rec2.width) || (rec1.x <= rec2.x && rec1.x + rec1.width >= rec2.x + rec2.width))
            return true;
    return false;
}

//Custom function to check a general collision between two rectangles
bool checkAllCollisionsRectangles(Rectangle rec1, Rectangle rec2) {
    //If they are colliding, returns true
    if (checkCollisionRectanglesLeft(rec1, rec2) == true)   
        return true;
    if (checkCollisionRectanglesRight(rec1, rec2) == true)
        return true;
    if (checkCollisionRectanglesBottom(rec1, rec2) == true)
        return true;
    if (checkCollisionRectanglesTop(rec1, rec2) == true)
        return true;
    return false;   //If they are not colliding, returns false
}

//Custom function to handle player/environment collision, adjusts player speed to avoid collision and can set isPlayerOnGround and canPlayerJump flags
void handlePlayerEnvironmentCollision(Rectangle *environmentHitbox, int environmentHitboxCount, entityData *player, bool *isPlayerOnGround, bool *canPlayerJump) {
    Vector2 newPlayerSpeed = {player->speed_x, player->speed_y};    //New player speed is initialized as equal to the original
    for (int i = 0; i < environmentHitboxCount; i++) {            //For every in use environment hitbox, tests player/hitbox collisions
        //Check left and right collisions
        if(checkCollisionRectanglesLeft((Rectangle){player->x + player->speed_x, player->y, player->width, player->height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
            newPlayerSpeed.x = environmentHitbox[i].x + environmentHitbox[i].width - player->x ;   //Update new player X speed if a collision was detected
        if(checkCollisionRectanglesRight((Rectangle){player->x + player->speed_x, player->y, player->width, player->height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
            newPlayerSpeed.x = environmentHitbox[i].x - player->x  - player->width;   //Update new player X speed if a collision was detected
        
        //Check top and bottom collisions (takes into account new player X position to prevent the ambiguous 45° collision case)
        if (checkCollisionRectanglesTop((Rectangle){player->x + newPlayerSpeed.x, player->y + player->speed_y, player->width, player->height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height})) {
            newPlayerSpeed.y = environmentHitbox[i].y + environmentHitbox[i].height - player->y;  //Update new player Y speed if a collision was detected
            *canPlayerJump = false;     //If player collides top (head), resets jump, so the player needs to reach the ground before jumping again
        }
        if (checkCollisionRectanglesBottom((Rectangle){player->x + newPlayerSpeed.x, player->y + player->speed_y, player->width, player->height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height})) {
            newPlayerSpeed.y = environmentHitbox[i].y - player->y - player->height;   //Update new player Y speed if a collision was detected
            *isPlayerOnGround = true;   //If player collides bottom (feet), the player has reached the ground
        }      
    }
    player->x += newPlayerSpeed.x;      //Update player X position after changes
    player->y += newPlayerSpeed.y;      //Update player Y position after changes
    player->speed_x = newPlayerSpeed.x;     //Update player X speed after changes
    player->speed_y = newPlayerSpeed.y;     //Update player Y speed after changes
}

//Custom function to handle enemy/environment collision, does not set any flags
void handleEnemyEnvironmentCollision(Rectangle *environmentHitbox, int environmentHitboxCount, entityData *enemy) {
    for (int j = 0; j < MAX_ENEMIES; j++) {  //For every enemy
        if (enemy[j].type == -1)    //If enemy is dead, does not apply collision
            continue;
        Vector2 newEnemySpeed = {enemy[j].speed_x, enemy[j].speed_y}; 
        if (enemy[j].type == 1) {    //If enemy is a walking one, apply collision with the environment
            for (int i = 0; i < environmentHitboxCount; i++) {    //For every in use environment hitbox, tests enemy/hitbox collisions
                //Check left and right collisions
                if(checkCollisionRectanglesLeft((Rectangle){enemy[j].x + enemy[j].speed_x, enemy[j].y, enemy[j].width, enemy[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newEnemySpeed.x = environmentHitbox[i].x + environmentHitbox[i].width - enemy[j].x ;  
                if(checkCollisionRectanglesRight((Rectangle){enemy[j].x + enemy[j].speed_x, enemy[j].y, enemy[j].width, enemy[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newEnemySpeed.x = environmentHitbox[i].x - enemy[j].x  - enemy[j].width;   
                
                //Check top and bottom collisions
                if (checkCollisionRectanglesTop((Rectangle){enemy[j].x + newEnemySpeed.x, enemy[j].y + enemy[j].speed_y, enemy[j].width, enemy[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newEnemySpeed.y = environmentHitbox[i].y + environmentHitbox[i].height - enemy[j].y;  
                if (checkCollisionRectanglesBottom((Rectangle){enemy[j].x + newEnemySpeed.x, enemy[j].y + enemy[j].speed_y, enemy[j].width, enemy[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newEnemySpeed.y = environmentHitbox[i].y - enemy[j].y - enemy[j].height;     
            }
        }
        enemy[j].x += newEnemySpeed.x;      
        enemy[j].y += newEnemySpeed.y;    
        enemy[j].speed_x = newEnemySpeed.x;     
        enemy[j].speed_y = newEnemySpeed.y;  
    }  
}

//Custom function to handle coins/chests collisions
void handleCoinEnvironmentCollision(Rectangle *environmentHitbox, int environmentHitboxCount, entityData *coin) {
    for (int j = 0; j < MAX_COINS; j++) { //For every coin
        if (coin[j].type == -1)     //If coin was collected/not spawned, does not apply collision
            continue;
        Vector2 newCoinSpeed = {coin[j].speed_x, coin[j].speed_y}; 
        if (coin[j].type == 1) {    //If coin is magnetic, does not apply collision
            for (int i = 0; i < environmentHitboxCount; i++) {  //For every in use environment hitbox, tests coin/hitbox collisions
                //Check left and right collisions
                if(checkCollisionRectanglesLeft((Rectangle){coin[j].x + coin[j].speed_x, coin[j].y, coin[j].width, coin[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newCoinSpeed.x = environmentHitbox[i].x + environmentHitbox[i].width - coin[j].x ;  
                if(checkCollisionRectanglesRight((Rectangle){coin[j].x + coin[j].speed_x, coin[j].y, coin[j].width, coin[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newCoinSpeed.x = environmentHitbox[i].x - coin[j].x  - coin[j].width;   
                
                //Check top and bottom collisions
                if (checkCollisionRectanglesTop((Rectangle){coin[j].x + newCoinSpeed.x, coin[j].y + coin[j].speed_y, coin[j].width, coin[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newCoinSpeed.y = environmentHitbox[i].y + environmentHitbox[i].height - coin[j].y;  
                if (checkCollisionRectanglesBottom((Rectangle){coin[j].x + newCoinSpeed.x, coin[j].y + coin[j].speed_y, coin[j].width, coin[j].height}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
                    newCoinSpeed.y = environmentHitbox[i].y - coin[j].y - coin[j].height;     
            }
        }
        coin[j].x += newCoinSpeed.x;      
        coin[j].y += newCoinSpeed.y;    
        coin[j].speed_x = 0;    //Avoid yeeeet coins
        coin[j].speed_y = newCoinSpeed.y;  
    }  
}

//Custom function to load any in use environment hitbox, changes when more map is generated
void loadEnvironmentHitboxes(Rectangle *inUseEnvironmentHitbox, int *environmentHitboxCount, int *mapScreenOffsetX) {
    //Environment hitboxes lookup table
    //platform_1 screen hitboxes (Screen 0)   
    //This screen has 10 hitboxes and they are defined as follows:
    Rectangle platform_1_hitboxes[platform_1_count] = {
    {0 + mapScreenOffsetX[0], 550, 1280, 170}, {552 + mapScreenOffsetX[0], 441, 178, 34}, {283 + mapScreenOffsetX[0], 365, 178, 34},
    {794 + mapScreenOffsetX[0], 332, 178, 34}, {505 + mapScreenOffsetX[0], 229, 178, 34}, {508 + mapScreenOffsetX[0], -189, 178, 34},
    {737 + mapScreenOffsetX[0], -66, 222, 30}, {0 + mapScreenOffsetX[0], 152, 456, 30}, {205 + mapScreenOffsetX[0], 37, 456, 30},
    {824 + mapScreenOffsetX[0], 152, 456, 30}};    

    //platform_2 screen hitboxes (Screen 1)    
    //This screen has 8 hitboxes and they are defined as follows:
    Rectangle platform_2_hitboxes[platform_2_count] = {
    {0 + mapScreenOffsetX[1], 550, 809, 170}, {809 + mapScreenOffsetX[1], 309, 60, 270}, {869 + mapScreenOffsetX[1], 550, 411, 170},
    {237 + mapScreenOffsetX[1], 257, 456, 30}, {66 + mapScreenOffsetX[1], 350, 178, 33}, {315 + mapScreenOffsetX[1], 441, 178, 33},
    {908 + mapScreenOffsetX[1], 341, 178, 33}, {1034 + mapScreenOffsetX[1], 442, 178, 33}}; 

    //platform_3 screen hitboxes (Screen 2)   
    //This screen has 5 hitboxes and they are defined as follows:
    Rectangle platform_3_hitboxes[platform_3_count] = {
    {0 + mapScreenOffsetX[2], 489, 1280, 220}, {106 + mapScreenOffsetX[2], 284, 384, 30}, {576 + mapScreenOffsetX[2], 360, 384, 30},
    {418 + mapScreenOffsetX[2], 166, 384, 30}, {896 + mapScreenOffsetX[2], 78, 384, 30}}; 

    //platform_4 screen hitboxes (Screen 3)   
    //This screen has 6 hitboxes and they are defined as follows:
    Rectangle platform_4_hitboxes[platform_4_count] = {
    {0 + mapScreenOffsetX[3], 489, 1280, 220}, {0 + mapScreenOffsetX[3], -144, 636, 30}, {565 + mapScreenOffsetX[3], -22, 237, 30},
    {751 + mapScreenOffsetX[3], 114, 237, 30}, {569 + mapScreenOffsetX[3], 233, 237, 30}, {753 + mapScreenOffsetX[3], 360, 237, 30}};  

    //platform_5 screen hitboxes (Screen 4)
    //This screen has 6 hitboxes and they are defined as follows:  
    Rectangle platform_5_hitboxes[platform_5_count] = {
    {0 + mapScreenOffsetX[4], 550, 1020, 170}, {1020 + mapScreenOffsetX[4], 489, 260, 70}, {824 + mapScreenOffsetX[4], 152, 455, 30},
    {137 + mapScreenOffsetX[4], 442, 355, 30}, {576 + mapScreenOffsetX[4], 360, 385, 30}, {507 + mapScreenOffsetX[4], 231, 178, 30}};

    //Load screens hitboxes (up to 3 screens at the same time)
    int hitbox = 0, i = 0;
    //Screen 0
    if (mapScreenOffsetX[0] != -1) {
        for (i = hitbox; i < platform_1_count + hitbox; i++)
            inUseEnvironmentHitbox[i] = platform_1_hitboxes[i];
        hitbox = i;
    }
    //Screen 1
    if (mapScreenOffsetX[1] != -1) {
        for (i = hitbox; i < platform_2_count + hitbox; i++)
            inUseEnvironmentHitbox[i] = platform_2_hitboxes[i - hitbox];
        hitbox = i;
    }
    //Screen 2
    if (mapScreenOffsetX[2] != -1) {
        for (i = hitbox; i < platform_3_count + hitbox; i++)
            inUseEnvironmentHitbox[i] = platform_3_hitboxes[i - hitbox];
        hitbox = i;
    }
    //Screen 3
    if (mapScreenOffsetX[3] != -1) {
        for (i = hitbox; i < platform_4_count + hitbox; i++)
            inUseEnvironmentHitbox[i] = platform_4_hitboxes[i - hitbox];
        hitbox = i;
    }
    //Screen 4
    if (mapScreenOffsetX[4] != -1) {
        for (i = hitbox; i < platform_5_count + hitbox; i++)
            inUseEnvironmentHitbox[i] = platform_5_hitboxes[i - hitbox];
        hitbox = i;
    }
    //Update total hitboxes in use
    *environmentHitboxCount = hitbox;
}

//Custom function that generates and draw new map screens as the player moves, also loads hitboxes from these screens to be used in the player/environment collision system
void generateMap(int *mapLeftScreen, int *mapMiddleScreen, int *mapRightScreen, Texture2D *mapScreen, int *mapScreenOffsetX, Rectangle *inUseEnvironmentHitbox, int *environmentHitboxCount, float *playerMiddleScreenOffsetX) {
    //Player has gone 1 screen to the right
    if (*playerMiddleScreenOffsetX >= 1280) {   
        *playerMiddleScreenOffsetX = 0;         //Resets player middle screen offset
        //Sets previous left screen to "not used" state
        mapScreenOffsetX[*mapLeftScreen] = -1;
        //Shifts screens to the left
        *mapLeftScreen = *mapMiddleScreen;      //Left is now the previous middle one
        *mapMiddleScreen = *mapRightScreen;     //Middle is now the previous right one
        while (1) {                             //Generates a new right screen different from both middle and left ones
            *mapRightScreen = GetRandomValue(0, 4);
            if (*mapRightScreen != *mapMiddleScreen && *mapRightScreen != *mapLeftScreen)
                break;     
        }
        mapScreenOffsetX[*mapRightScreen] = mapScreenOffsetX[*mapMiddleScreen] + 1280;  //New right screen offset 
    }
    //Player has gone 1 screen to the left
    if (*playerMiddleScreenOffsetX <= -1280) {  
        *playerMiddleScreenOffsetX = 0;         //Resets player middle screen offset
        //Sets previous right screen to "not used" state
        mapScreenOffsetX[*mapRightScreen] = -1;
        //Shifts screens to the right
        *mapRightScreen = *mapMiddleScreen;     //Right is now the previous middle one
        *mapMiddleScreen = *mapLeftScreen;      //Middle is now the previous left one
        while (1) {                             //Generates new right screen different from both middle and right ones
            *mapLeftScreen = GetRandomValue(0, 4);
            if (*mapLeftScreen != *mapMiddleScreen && *mapLeftScreen != *mapRightScreen)
                break;     
        }
        mapScreenOffsetX[*mapLeftScreen] = mapScreenOffsetX[*mapMiddleScreen] - 1280;  //New left screen offset 
    }
    //Draw generated map
    for (int i = 0; i < 5; i++)
        if (mapScreenOffsetX[i] != -1) //Won't draw "not used" ones (-1 as offset)
            DrawTexture(mapScreen[i], mapScreenOffsetX[i], -720, (Color){180, 180, 180, 255});
    //Load hitboxes for the given map
    loadEnvironmentHitboxes(inUseEnvironmentHitbox, environmentHitboxCount, mapScreenOffsetX);
}

//Custom function to draw a mountain and a sky background with parallax effect
void drawParallaxBackground(float *parallaxOffsetMountainX, float *newOffsetMountainX, float *parallaxOffsetSkyX, float *newOffsetSkyX, entityData player, Texture2D *background) {
    //Divisions by 6 and 12 determines parallax speed/intensity
    if (*parallaxOffsetMountainX + *parallaxOffsetMountainX/6 >= 1280)                              //If player has traveled one screen to the right
        *newOffsetMountainX += 1280 - *parallaxOffsetMountainX/6, *parallaxOffsetMountainX = 0;     //Updates mountain position
    if (*parallaxOffsetMountainX + *parallaxOffsetMountainX/6 <= -1280)                             //If player has traveled one screen to the left
        *newOffsetMountainX -= 1280 + *parallaxOffsetMountainX/6, *parallaxOffsetMountainX = 0;     //Updates mountain position
    
    if (*parallaxOffsetSkyX + *parallaxOffsetSkyX/12 >= 1280)                           //If player has traveled one screen to the right
        *newOffsetSkyX += 1280 - *parallaxOffsetSkyX/12, *parallaxOffsetSkyX = 0;       //Updates sky position
    if (*parallaxOffsetSkyX + *parallaxOffsetSkyX/12 <= -1280)                          //If player has traveled one screen to the left
        *newOffsetSkyX -= 1280 + *parallaxOffsetSkyX/12, *parallaxOffsetSkyX = 0;       //Updates sky position

    DrawTexture(background[1], -1280 + *newOffsetSkyX - *parallaxOffsetSkyX/12, player.y - 500, WHITE); //Draw sky
    DrawTexture(background[0], -1280 + *newOffsetMountainX - *parallaxOffsetMountainX/6, player.y  - 500, (Color){120, 120, 120, 255}); //Draw mountain
}

//Custom function to draw the player walking animation
void drawPlayer(entityData player, Texture2D *playerWalk, bool *walkLeft, int *walkAnimCount) {
    if (player.speed_x > 0)  //Flip the walking animation based on player facing direction
        *walkLeft = false;
    if (player.speed_x < 0)
        *walkLeft = true;
    //Draw the animation frames (-11 and -6 * *walkLeft, adjusts the left sprite position to match player hitbox)
    if (player.speed_x == 0)     //Idle
        DrawTexture(playerWalk[0 + *walkLeft], player.x - 14 * *walkLeft, player.y, WHITE), *walkAnimCount = 1; 
    else if (player.speed_y != 0 && player.speed_x != 0)  //Jumping (locks the animation and sets new walkAnimCount to continue the animation when the player hits the ground)
        DrawTexture(playerWalk[4 + *walkLeft], player.x - 9 * *walkLeft, player.y, WHITE), *walkAnimCount = 12; 
    else for (int i = 1; i < 7; i++)    //Walking
        if (*walkAnimCount >= 6 * i - 6 && *walkAnimCount < 6 * i) //0 - 6; 6 - 12; 12 - 18; ...
            DrawTexture(playerWalk[2 * i + *walkLeft], player.x - 9 * *walkLeft, player.y, WHITE);
    //Reset the counter
    *walkAnimCount += 1;
    if (*walkAnimCount > 35)
        *walkAnimCount = 0;
}

//Custom function to draw chests/coins
void drawChestCoins(entityData *coin, entityData *chest, Texture2D *simpleIcons, Texture2D *skillHUD, Font pixellariFont) {
    //Draw coins  
    for (int i = 0; i < MAX_COINS; i++)
        if (coin[i].type != -1)
            DrawTexture(simpleIcons[2], coin[i].x, coin[i].y, WHITE);
    //Draw chests 
    for (int i = 0; i < MAX_CHESTS; i++) {
        //Locked chest
        if (chest[i].type == 1) {   
            DrawTexture(simpleIcons[3], chest[i].x, chest[i].y, WHITE);         //Chest locked
            DrawTexture(simpleIcons[5], chest[i].x, chest[i].y - 50, WHITE);    //Coin icon
            DrawTextEx(pixellariFont, TextFormat("%d", chest[i].price), (Vector2){chest[i].x + 26, chest[i].y - 50}, 20, 0, (Color){226, 180, 54, 255});    //Price
        }
        //Open chest
        if (chest[i].type == 2) {
            DrawTexture(simpleIcons[4], chest[i].x, chest[i].y, WHITE);         //Chest open
            if (chest[i].item != -1)                                            //Open with an item inside
                DrawTexture(skillHUD[5 + chest[i].item], chest[i].x - 45, chest[i].y - 70, WHITE);  //Draw item info
        }
    }
}

//Custom function to spawn coins upon an enemy's death
void spawnCoins(entityData *coin, entityData deadEnemy, playerStats playerStatsHUD) {
    //Get a random amount of coins (type 1 enemies give more coins)
    int coinAmount;
    if (deadEnemy.type == 1)
        coinAmount = GetRandomValue(2*playerStatsHUD.baseLuck, 4*playerStatsHUD.baseLuck);  //Number of coins to spawn
    else
        coinAmount = GetRandomValue(1*playerStatsHUD.baseLuck, 2*playerStatsHUD.baseLuck); 
    //Spawn coins
    for (int i = 0, j = 0; i < MAX_COINS && j < coinAmount; i++) {    //For every coin to spawn
        if (coin[i].type != -1)
            continue;
        //Generate general coin data
        coin[i].type = 1;
        coin[i].width = 8;
        coin[i].height = 8;
        coin[i].speed_x = 0;
        coin[i].speed_y = -5;
        coin[i].x = GetRandomValue(deadEnemy.x, deadEnemy.x + deadEnemy.width); //Spawns at a random location in the enemy's body (though not too close to the ground)
        coin[i].y = GetRandomValue(deadEnemy.y, deadEnemy.y + deadEnemy.height/2);
        coin[i].spawnTime = playerStatsHUD.elapsedTime;
        j++;    //One coin spawned
    }
}

//Custom function to draw any active enemy and handle its mechanics
void drawHandleEnemies(Sound *sfx, entityData *enemy, Texture2D *enemy2D, int *deadCount, entityData player, playerStats *playerStatsHUD, entityData *coin) {
    for (int j = 0; j < MAX_ENEMIES; j++) {          //For every enemy
        if (enemy[j].type == -1)            //Doesn't update dead enemies
            continue;
        if (enemy[j].health <= 0)  {        //If an enemy dies
            if (enemy[j].type == 1)         //Increases player xp 
                playerStatsHUD->xp += 40;   //Player kills type 1 enemy
            else
                playerStatsHUD->xp += 30;   //Player kills type 2 enemy
            PlaySound(sfx[4]);              //Play death sound
            spawnCoins(coin, enemy[j], *playerStatsHUD); //Spawn coins upon an enemy's death
            enemy[j].type = -1;             //Sets enemy to "dead" state
            *deadCount += 1;
            playerStatsHUD->enemiesKilled += 1;
        }
        //Reset stray enemies (too far from the player)
        if (player.x - enemy[j].x > 1920 || player.x - enemy[j].x < -1920 || enemy[j].y > 720)  //1.5x screens too far
            enemy[j].type = -1, *deadCount += 1;
        //Draw enemies
        DrawTexture(enemy2D[enemy[j].sprite], enemy[j].x, enemy[j].y, WHITE);   //Enemy
        DrawRectangle(enemy[j].x, enemy[j].y - 20, (enemy[j].health/enemy[j].maxHealth)*enemy[j].width, 12, (Color){255, 0, 0, 255});   //Enemy HP
        DrawRectangleLinesEx((Rectangle){enemy[j].x, enemy[j].y - 20, enemy[j].width, 12}, 2, WHITE);  
    }
}

//Custom function to draw the elapsed time on the player HUD
void drawElapsedTime(int *frameCount, playerStats *playerStatsHUD, Font pixellariFont) {
    //Counting 1 second every 60 frames
    if (*frameCount > 59)
        *frameCount = 0, playerStatsHUD->elapsedTime++;
    *frameCount += 1;
    //Calculate minutes and seconds
    int minutes = playerStatsHUD->elapsedTime/60;
    int seconds = playerStatsHUD->elapsedTime - minutes*60;
    //Draw time on screen 
    DrawTextEx(pixellariFont, TextFormat("%02d : %02d", minutes, seconds), (Vector2){1168+3, 31+3}, 30, 0, (Color){0, 0, 0, 127});  //Drop shadow  
    DrawTextEx(pixellariFont, TextFormat("%02d : %02d", minutes, seconds), (Vector2){1168, 31}, 30, 0, WHITE);
}

//Custom function to draw the main player HUD
bool drawHUD(Sound *sfx, Font pixellariFont, Texture2D *mainHUD, Texture2D *skillHUD, playerStats *playerStatsHUD, entityData player, int lastDamageTaken, int *levelUpAnim, bool playerOnChest) {
    //Back HUD layer
    DrawTexture(mainHUD[0], 495, 585, WHITE); 
    //LEVEL UP
    while (playerStatsHUD->xp >= playerStatsHUD->maxXp) {   //If player has leveled up
        playerStatsHUD->level += 1;                         //Adds 1 level
        playerStatsHUD->xp -= playerStatsHUD->maxXp;        //Resets xp
        playerStatsHUD->maxXp *= 1 + ((float)playerStatsHUD->level/12); //Increases xp needed for next level
        PlaySound(sfx[7]);
        *levelUpAnim = 1;
    }
    //LEVEL AND GOLD
    //LEVEL UP ANIMATION
    if (*levelUpAnim > 0) {
        DrawTextExCenterHover(pixellariFont, "LEVEL UP", (Vector2){613.5, 490 - *levelUpAnim/3}, 25, 0, 
        (Color){10, 200 - *levelUpAnim*3, 0 + *levelUpAnim*3, 255 - *levelUpAnim*4.25}, (Color){10, 200 - *levelUpAnim*3, 0 + *levelUpAnim*3, 255 - *levelUpAnim*4.25});
        *levelUpAnim += 1;
        if (*levelUpAnim > 60)  //Animation plays for 1 second
            *levelUpAnim = 0;
    }
    DrawTextExCenterHover(pixellariFont, TextFormat("%d", playerStatsHUD->level), (Vector2){525, 630}, 25, 0, WHITE, WHITE);        //LVL     
    DrawTextEx(pixellariFont, TextFormat("%04d", playerStatsHUD->gold), (Vector2){620, 585}, 20, 0, (Color){226, 180, 54, 255});    //GOLD
    //Skill 1
    DrawTexture(skillHUD[0], 565, 609, WHITE); //SKILL 1 ALWAYS AVAILABLE   
    //Skill 2
    if (playerStatsHUD->CooldownSkill2 - playerStatsHUD->elapsedTime > 0) {
        DrawTexture(skillHUD[1], 624, 609, (Color){127, 127, 127, 255}); //SKILL 2 COOLDOWN
        DrawTextExCenterHover(pixellariFont, TextFormat("%d", playerStatsHUD->CooldownSkill2 - playerStatsHUD->elapsedTime), (Vector2){643, 630}, 25, 0, WHITE, WHITE);
    } else {
        DrawTexture(skillHUD[1], 624, 609, WHITE); //SKILL 2 AVAILABLE
        playerStatsHUD->CooldownSkill2 = 0;
    }
    //Skill 3
    if (playerStatsHUD->CooldownSkill3 - playerStatsHUD->elapsedTime > 0) {
        DrawTexture(skillHUD[2], 683, 609, (Color){127, 127, 127, 255}); //SKILL 3 COOLDOWN
        DrawTextExCenterHover(pixellariFont, TextFormat("%d", playerStatsHUD->CooldownSkill3 - playerStatsHUD->elapsedTime), (Vector2){702, 630}, 25, 0, WHITE, WHITE);
    } else {
        DrawTexture(skillHUD[2], 683, 609, WHITE); //SKILL 3 AVAILABLE
        playerStatsHUD->CooldownSkill3 = 0;
    }
    //Draw current equipped item
    if (playerStatsHUD->itemEquipped != -1) {
        DrawTexture(mainHUD[2], 742, 609, (Color){255 - 128*playerOnChest, 255 - 128*playerOnChest, 255 - 128*playerOnChest, 255});
        DrawTexture(skillHUD[2 + playerStatsHUD->itemEquipped], 742, 609, (Color){255 - 128*playerOnChest, 255 - 128*playerOnChest, 255 - 128*playerOnChest, 255});
    }
    //Draw current active item and its timer
    if (playerStatsHUD->itemActive != -1) {
        DrawTexture(skillHUD[2 + playerStatsHUD->itemActive], 100, 609, WHITE);
        if (playerStatsHUD->itemTimer - playerStatsHUD->elapsedTime > 0)    //Item effect active
            DrawTextExCenterHover(pixellariFont, TextFormat("%d", playerStatsHUD->itemTimer - playerStatsHUD->elapsedTime), (Vector2){160, 630}, 25, 0, WHITE, WHITE);
        else    
            playerStatsHUD->itemActive = -1;                                //Item effect expired
    }       
    //Health and XP
    //MAX HEALTH CAP
    if (playerStatsHUD->health > playerStatsHUD->maxHealth)     //If player health is above the max health
        playerStatsHUD->healthAnim = playerStatsHUD->health = playerStatsHUD->maxHealth;    //Sets health limit to max health

    //Reduces red health bar after 1 second without taking damage (thus animating it)
    if (playerStatsHUD->healthAnim > playerStatsHUD->health && playerStatsHUD->elapsedTime - lastDamageTaken >= 1)
        playerStatsHUD->healthAnim -= 0.7;
    DrawRectangle(565, 658, playerStatsHUD->healthAnim*1.55, 9, (Color){255, 0, 30, 255});                      //RED HEALTH
    DrawRectangle(565, 658, playerStatsHUD->health/playerStatsHUD->maxHealth*155 , 9, (Color){0, 220, 0, 255}); //HEALTH
    DrawRectangle(565, 675, playerStatsHUD->xp/playerStatsHUD->maxXp*155, 4, (Color){0, 136, 255, 255});        //XP   

    //Front HUD layer
    DrawTexture(mainHUD[1], 495, 585, WHITE); 

    //PLAYER DIES
    if (playerStatsHUD->health <= 0)    //If player has died
        return true;    //Returns true
    else
        return false;   //Player hasn't died, returns false                                 
}

//Custom function to handle player coin/chest mechanics
void handlePlayerCoinChest(Sound *sfx, entityData player, playerStats *playerStatsHUD, entityData *coin, entityData *chest, bool *playerOnChest, Font pixellariFont, overlayVariables *overlayData) {
    //COIN MECHANICS
    float hypotenuse, opposite, adjacent, oppositeAbs, adjacentAbs; //Variables used to make coins fly towards the player
    *playerOnChest = false; //Player on chest flag
    for (int i = 0; i < MAX_COINS; i++) { //For every coin
        if (coin[i].type == -1)
            continue;
        //Check if player has collected any coins
        if (checkAllCollisionsRectangles((Rectangle){player.x, player.y, player.width, player.height},  //If player and coins are colliding
        (Rectangle){coin[i].x, coin[i].y, coin[i].width, coin[i].height}) == true) {
            coin[i].type = -1;
            playerStatsHUD->gold += 1;
            playerStatsHUD->totalGold += 1;
            PlaySound(sfx[6]);
        }
        //If a coin has been on ground for too long, it will fly towards the player
        if (coin[i].type == 1 && playerStatsHUD->elapsedTime - coin[i].spawnTime > 5)
            coin[i].type = 2;
        //Make coin fly towards the player
        if (coin[i].type == 2) {
            //If the coin has reached the player, the coin stops
            if (coin[i].x + coin[i].width/2 > player.x && coin[i].x + coin[i].width/2 < player.x + player.width)
                if (coin[i].y + coin[i].height/2 > player.y && coin[i].y + coin[i].height/2 < player.y + player.height) {
                    coin[i].speed_x = 0;
                    coin[i].speed_y = 0;
                    continue;
                }
            //Calculates distance from player using a right triangle
            oppositeAbs = opposite = player.x + player.width/2 - coin[i].x - coin[i].width/2;         
            adjacentAbs = adjacent = player.y + player.height/2 - coin[i].y - coin[i].height/2;
            hypotenuse = sqrt(pow(opposite, 2) + pow(adjacent, 2));
            //Calculates absolute value for the opposite and adjacent sides
            if (opposite < 0)       
                oppositeAbs *= -1;
            if (adjacent < 0)
                adjacentAbs *= -1;
            //Determines X and Y speeds
            if (oppositeAbs <= adjacentAbs) {               //If X distance is smaller than Y distance
                coin[i].speed_y = 15;                       //Caps Y speed at 15 px/frame
                if (adjacent < 0)   
                    coin[i].speed_y *= -1;                  //Inverts Y movement direction if below the player
                coin[i].speed_x = (opposite/hypotenuse)*15; //X speed is equal to the sin of theta times 15 px/frame 
            } else {                                        //If Y distance is smaller than X distance
                coin[i].speed_x = 15;                       //Caps X speed at 15 px/frame          
                if (opposite < 0)
                    coin[i].speed_x *= -1;                  //Inverts X movement direction if to the right of the player
                coin[i].speed_y = (adjacent/hypotenuse)*15; //Y speed is equal to the cos of theta times 15 px/frame
            }
        }
        //Reset stray coins (too far from the player)
        if (player.x - coin[i].x > 2560 || player.x - coin[i].x < -2560)    //2x screens too far
            coin[i].x = coin[i].y = coin[i].speed_x = coin[i].speed_y = 0, coin[i].type = -1;
    }
    //CHEST MECHANICS
    for (int i = 0; i < MAX_CHESTS; i++) {   //For every chest
        if (chest[i].type == -1)
            continue;
        //Reset stray chests (too far from the player)
        if (player.x - chest[i].x > 1920 || player.x - chest[i].x < -1920)  //1.5x screens too far
            chest[i].type = -1;
        //Player is close to a chest
        if (checkAllCollisionsRectangles((Rectangle){player.x, player.y, player.width, player.height},  //If player and chest are colliding
        (Rectangle){chest[i].x, chest[i].y, chest[i].width, chest[i].height}) == true) {
            //Prevent activating item while on a chest (as long as the chest has an item)
            if (chest[i].item != -1) {
                *playerOnChest = true; 
                if (chest[i].type == 1)
                    DrawTextEx(pixellariFont, "E to Buy", (Vector2){chest[i].x - 12, chest[i].y - 25}, 20, 0, (Color){226, 180, 54, 255});
                else 
                    DrawTextEx(pixellariFont, "E to Equip", (Vector2){chest[i].x - 20, chest[i].y - 25}, 20, 0, (Color){226, 180, 54, 255});
            }
            //Chest actions (press E)
            if (IsKeyPressed(KEY_E)) {
                if (chest[i].type == 1) {                                           //If it is a locked chest
                    if (playerStatsHUD->gold >= chest[i].price)                     //And the player can buy it
                        playerStatsHUD->gold -= chest[i].price, chest[i].type = 2;  //Chest is now open and the player has spent gold
                    else
                        sendOverlayInfo("Not enough gold", overlayData);            //Not enough gold
                } else {                                                                    //If it is an open chest
                    if (chest[i].item != -1)                                                //And the chest has an item inside
                        playerStatsHUD->itemEquipped = chest[i].item, chest[i].item = -1;   //Item is equipped and chest is left empty
                }
            }
        }
    }
}

//Custom function to handle player damage inflicted by enemies
void handlePlayerDamage(Sound *sfx, entityData player, playerStats *playerStatsHUD, entityData *enemy, int *lastDamageTaken, bool isDashing) {
    //Player takes damage
    if (playerStatsHUD->elapsedTime - *lastDamageTaken >= 1 && isDashing == false) {    //If 1 second has elapsed after last damage was taken 
        for (int j = 0; j < MAX_ENEMIES; j++) {                      //For every enemy           //and player is not dashing, can take damage again
            if (enemy[j].type == -1)
                continue;
            if (checkAllCollisionsRectangles((Rectangle){player.x, player.y, player.width, player.height},  //If player and enemy are colliding
            (Rectangle){enemy[j].x, enemy[j].y, enemy[j].width, enemy[j].height}) == true) {
                *lastDamageTaken = playerStatsHUD->elapsedTime;    //Updates lastDamageTaken timer
                if (enemy[j].type == 1) {
                    playerStatsHUD->health -= 1.5*playerStatsHUD->baseDamage;   //Takes damage from walking enemies (type 1)
                    PlaySound(sfx[5]);
                } else {
                    playerStatsHUD->health -= 2*playerStatsHUD->baseDamage;     //Takes damage from flying enemies (type 2)
                    PlaySound(sfx[5]);
                }
            }
        }
    }
    //Player health regen
    if (playerStatsHUD->elapsedTime - *lastDamageTaken >= 5) {      //5 seconds elapsed with no damage taken
        playerStatsHUD->health += playerStatsHUD->baseHealthRegen;  //1.8 HP regen per second (0.03 per frame @ 60 fps)
        playerStatsHUD->healthAnim += playerStatsHUD->baseHealthRegen;
    }
}

//Custom function to handle enemy damage inflicted by the player
int handleEnemyDamage(bool isLaser, Texture2D *simpleIcons, entityData player, entityData *enemy, playerStats playerStatsHUD, Rectangle damageHitbox) {  
    //Variables used in the simple shot damage calculation
    int enemiesHit[MAX_ENEMIES], totalHit = 0, closestEnemy;
    float smallestDistance, distance;
    bool onPlayerLeft = true;
    //Calculate enemy damage
    for (int j = 0; j < MAX_ENEMIES; j++) {  //For every enemy
        if (enemy[j].type == -1)        //If enemy is not dead
            continue;
        if (isLaser == true) {          //If damage is coming from laser
            if (checkAllCollisionsRectangles((Rectangle){enemy[j].x, enemy[j].y, enemy[j].width, enemy[j].height}, damageHitbox) == true)
                enemy[j].health -= playerStatsHUD.baseDamage*3;    //Deals 3x base damage to all enemies hit by the laser
        } else {                        //Damage is coming from simple shot
            if (checkAllCollisionsRectangles((Rectangle){enemy[j].x, enemy[j].y, enemy[j].width, enemy[j].height}, damageHitbox) == true)
                enemiesHit[totalHit] = j, totalHit++;   //Saves which enemies were hit    
        }
    }
    //If damage came from simple shot, checks which enemy is the closest to the player, it will be the only enemy to take damage 
    if (isLaser == false && totalHit > 0) {
        closestEnemy = enemiesHit[0];           //Closest enemy starts as the first one
        smallestDistance = player.x - enemy[closestEnemy].x;    //Smallest distance starts as the first one
        if (smallestDistance < 0)               //Check if the enemies are to the left of the player
            onPlayerLeft = false;               //If so, sets the "onPLayerLeft" flag
        if (onPlayerLeft == false)                      
            smallestDistance *= -1;             //Removes negative sign if enemies are to the right of the player
        for (int i = 1; i < totalHit; i++) {    //Checks which enemy is the closest one
            distance = player.x - enemy[enemiesHit[i]].x;
            if (onPlayerLeft == false)                      
                distance *= -1;
            if (distance <= smallestDistance)
                smallestDistance = distance, closestEnemy = enemiesHit[i];
        }
        //Deals damage to the closest enemy
        enemy[closestEnemy].health -= playerStatsHUD.baseDamage;   //Deals 1x base damage to the closest enemy
        //Returns closest enemy, so the handleShot function can draw the shot on the enemy's body
        return closestEnemy;  
    }
    return -1;  //No enemy hit by the simple shot, returns -1
}

//Custom function to draw the shot animation (skill 1)
void handleShot(int *closestEnemy, int *shotTimer, bool walkLeft, entityData player, Texture2D *simpleIcons, playerStats playerStatsHUD, entityData *enemy) {
    if (*shotTimer > 0 && *shotTimer < 7) { //If player has shot
        if (walkLeft == true) {
            DrawTexture(simpleIcons[1], player.x - 13 - 12, player.y + 22, WHITE);   //Draw shot animation left
            if (*shotTimer == 1)    //Deal damage to enemies on player's left side
                *closestEnemy = handleEnemyDamage(false, simpleIcons, player, enemy, playerStatsHUD, (Rectangle){player.x - 13 - 700, player.y + 22, 700, 15});
            if (*closestEnemy != -1)
                DrawTexture(simpleIcons[0], enemy[*closestEnemy].x + enemy[*closestEnemy].width, player.y + 22, WHITE);  //Draw shot animation on the hit enemy   
        } else {
            DrawTexture(simpleIcons[0], player.x + 40, player.y + 22, WHITE);        //Draw shot animation right
            if (*shotTimer == 1)    //Deal damage to enemies on player's right side
                *closestEnemy = handleEnemyDamage(false, simpleIcons, player, enemy, playerStatsHUD, (Rectangle){player.x + 40, player.y + 22, 700, 15});
            if (*closestEnemy != -1)
                DrawTexture(simpleIcons[1], enemy[*closestEnemy].x - 12, player.y + 22, WHITE);  //Draw shot animation on the hit enemy 
        }         
    }
    if (*shotTimer > 0) {       //Shot timer increment
        *shotTimer += 1;
        if (*shotTimer > 14 + 10 - playerStatsHUD.baseSpeed*2)  //For 6 frames the shot is visible, but the shot cooldown is higher
            *shotTimer = 0;
    }
}

//Custom function to draw the dashing animation and move the player (skill 3)
void handleDash(entityData *player, int *dashTimer, bool *isDashing, bool walkLeft, Texture2D *playerWalk) {
    if (*isDashing == true) {       //If player is dashing
        player->speed_y = 0;        //Prevent vertical movement
        if (walkLeft == true)
            player->speed_x = -20;  //Dash speed left
        else
            player->speed_x = 20;   //Dash speed right
        *dashTimer += 1;
        if (*dashTimer > 10)        //Dash timer
            *dashTimer = 0, *isDashing = false;
        for (int i = 0; i < *dashTimer; i += 2)     //Draw dashing animation
            DrawTexture(playerWalk[14 + walkLeft], player->x - i * 16 + i * 32 * walkLeft, player->y, (Color){255, 72 + i * 18, 0, 255 - i * 25});
    }
}

//Custom function to draw the laser animation (skill 2)
void handleLaser(bool *isLaserActive, int *laserTimer, bool walkLeft, entityData player, playerStats playerStats, entityData *enemy) {
    if (*isLaserActive == true) {   //If laser is active
        *laserTimer += 1;
        if (*laserTimer > 14)
            *laserTimer = 0, *isLaserActive = false;
        if (walkLeft == true) {     //If player is walking left
            if (*laserTimer == 1)       
                handleEnemyDamage(true, NULL, player, enemy, playerStats, (Rectangle){player.x - 13 - 315, player.y + 22, 315, 15});   //Deal damage to enemies to the left of the player
            if (*laserTimer < 7) {  //Initial laser animation (left)
                DrawRectangle(player.x - 13 - *laserTimer*45, player.y + 22, *laserTimer*45, 15, (Color){255, 0, 0, 255});      //Red 
                DrawRectangle(player.x - 13 - *laserTimer*45, player.y + 24, *laserTimer*45, 11, (Color){255, 102, 0, 255});    //Orange 
                DrawRectangle(player.x - 13 - *laserTimer*45, player.y + 26, *laserTimer*45, 7, (Color){255, 255, 0, 255});     //Yellow 
            }
            if (*laserTimer >= 7) { //Fading laser animation (left)
                DrawRectangle(player.x - 13 - 315, player.y + 22, 315 - (*laserTimer - 7)*45, 15, (Color){255, 0, 0, 255});
                DrawRectangle(player.x - 13 - 315, player.y + 24, 315 - (*laserTimer - 7)*45, 11, (Color){255, 102, 0, 255});
                DrawRectangle(player.x - 13 - 315, player.y + 26, 315 - (*laserTimer - 7)*45, 7, (Color){255, 255, 0, 255});
            }   
        } else {
            if (*laserTimer == 1)       
                handleEnemyDamage(true, NULL, player, enemy, playerStats, (Rectangle){player.x + 40, player.y + 22, 315, 15});        //Deal damage to enemies to the right of the player
            if (*laserTimer < 7) {  //Initial laser animation (right)
                DrawRectangle(player.x + 40, player.y + 22, *laserTimer*45, 15, (Color){255, 0, 0, 255});  
                DrawRectangle(player.x + 40, player.y + 24, *laserTimer*45, 11, (Color){255, 102, 0, 255});
                DrawRectangle(player.x + 40, player.y + 26, *laserTimer*45, 7, (Color){255, 255, 0, 255});
            }
            if (*laserTimer >= 7) { //Fading laser animation (right)
                DrawRectangle(player.x + 40 + (*laserTimer - 7)*45, player.y + 22, 315 - (*laserTimer - 7)*45, 15, (Color){255, 0, 0, 255});  
                DrawRectangle(player.x + 40 + (*laserTimer - 7)*45, player.y + 24, 315 - (*laserTimer - 7)*45, 11, (Color){255, 102, 0, 255});
                DrawRectangle(player.x + 40 + (*laserTimer - 7)*45, player.y + 26, 315 - (*laserTimer - 7)*45, 7, (Color){255, 255, 0, 255});
            } 
        }
    }
}

//Custom function to spawn chests on screen
void spawnChests(entityData *chest, entityData player, Rectangle *inUseEnvironmentHitbox, int environmentHitboxCount, playerStats playerStatsHUD) {
    int hitbox, i;  //Variables used while spawning chests
    float start_x;
    //Spawn up to 2 chests on screen
    for (i = 0; i < MAX_CHESTS; i++)
        if (chest[i].type != -1)    //If a chest is not being used, spawns a new one in its place
            continue;
        else {
            //Chooses between right and left sides of the player
            if (GetRandomValue(0, 1) == 1)
                start_x = GetRandomValue(player.x + 240, player.x + 660); //Spawns a chest close to the player (not too close)
            else   
                start_x = GetRandomValue(player.x - 200, player.x - 580);
            //Makes sure the chest spawns above a platform/ground
            do {
                hitbox = GetRandomValue(0, environmentHitboxCount - 1);     
            } while (!(start_x >= inUseEnvironmentHitbox[hitbox].x  && start_x <= inUseEnvironmentHitbox[hitbox].x + inUseEnvironmentHitbox[hitbox].width));
            //Makes sure the chest doesn't spawn on the edges of the platform/ground
            if (start_x >= inUseEnvironmentHitbox[hitbox].x + inUseEnvironmentHitbox[hitbox].width - 10 - 50)
                start_x -= (start_x - (inUseEnvironmentHitbox[hitbox].x + inUseEnvironmentHitbox[hitbox].width - 10 - 50));
            if (start_x <= inUseEnvironmentHitbox[hitbox].x + 10)
                start_x += (inUseEnvironmentHitbox[hitbox].x + 10 - start_x);
            //Generates general chest data
            chest[i].x = start_x;
            chest[i].y = inUseEnvironmentHitbox[hitbox].y - 50; //Spawns above a platform
            chest[i].width = 50;
            chest[i].height = 50;
            chest[i].type = 1;                                  //Locked chest
            chest[i].item = GetRandomValue(1, 3);               //Get a random item
            chest[i].price = 8 + 2*playerStatsHUD.level;        //Price scales with the player level
        }
}

//Custom function to spawn enemies on screen
void spawnEnemies(entityData *enemy, int *aliveCount, int *deadCount, int *lastEnemySpawn, entityData player, Rectangle *inUseEnvironmentHitbox, int environmentHitboxCount, playerStats playerStatsHUD) {
    int hitbox, i;  //Variables used while spawning enemies
    float start_x;
    //Spawn an enemy every second (max 10 enemies)
    if (*aliveCount < MAX_ENEMIES && playerStatsHUD.elapsedTime - *lastEnemySpawn >= ENEMY_SPAWN_INTERVAL) { 
        for (i = 0; i < MAX_ENEMIES; i++)
            if (enemy[i].type != -1)    //If an enemy is dead, spawns a new one in its place
                continue;
            else break;
        if ((enemy[i].type = GetRandomValue(1, 2)) == 1) {  //If enemy is from type 1
            //Chooses between right and left sides of the player
            if (GetRandomValue(0, 1) == 1)
                start_x = GetRandomValue(player.x + 140, player.x + 660); //Spawns an enemy close to the player (not too close)
            else   
                start_x = GetRandomValue(player.x - 100, player.x - 580);
            //Makes sure the enemy spawns above a platform/ground
            do {
                hitbox = GetRandomValue(0, environmentHitboxCount - 1);     
            } while (!(start_x >= inUseEnvironmentHitbox[hitbox].x  && start_x <= inUseEnvironmentHitbox[hitbox].x + inUseEnvironmentHitbox[hitbox].width));
            //Makes sure the enemy doesn't spawn on the edges of the platform/ground
            if (start_x >= inUseEnvironmentHitbox[hitbox].x + inUseEnvironmentHitbox[hitbox].width - 10 - 37)
                start_x -= (start_x - (inUseEnvironmentHitbox[hitbox].x + inUseEnvironmentHitbox[hitbox].width - 10 - 37));
            if (start_x <= inUseEnvironmentHitbox[hitbox].x + 10)
                start_x += (inUseEnvironmentHitbox[hitbox].x + 10 - start_x);
            //Generates general enemy data
            enemy[i].x = start_x;
            enemy[i].y = inUseEnvironmentHitbox[hitbox].y - 50; //Spawns above a platform
            enemy[i].width = 37;
            enemy[i].height = 50;
            enemy[i].speed_x = -1.5;                            //Initial speed
            enemy[i].speed_y = 0;
            enemy[i].health = ((float)playerStatsHUD.level/12 + 0.9) * 100; //Enemy health scales with the player level
            enemy[i].maxHealth = ((float)playerStatsHUD.level/12 + 0.9) * 100;
            enemy[i].hitboxStart = inUseEnvironmentHitbox[hitbox].x + 10;   //Keeps track of the spawning hitbox area
            enemy[i].hitboxEnd = inUseEnvironmentHitbox[hitbox].x + inUseEnvironmentHitbox[hitbox].width - 10; 
            enemy[i].sprite = GetRandomValue(4, 5);                         //Assign 1 out of 2 sprites for enemies of type 1
        } else {    //If enemy is from type 2
            //Spawns an enemy close to the player
            start_x = GetRandomValue(player.x - 100 - 640, player.x + 100 + 640);
            //Generates general enemy data
            enemy[i].x = start_x;
            enemy[i].y = -720;      //Spawns out of screen
            enemy[i].width = 40;
            enemy[i].height = 40;
            enemy[i].speed_x = 0;                           
            enemy[i].speed_y = 0;
            enemy[i].health = ((float)playerStatsHUD.level/12 + 0.9) * 80;      //Enemy health scales with the player level
            enemy[i].maxHealth = ((float)playerStatsHUD.level/12 + 0.9) * 80;
            enemy[i].sprite = GetRandomValue(0, 3);                             //Assign 1 out of 4 sprites for enemies of type 2
        }
        //Resets enemy spawn timer and changes alive/dead counters
        *lastEnemySpawn = playerStatsHUD.elapsedTime;
        *aliveCount += 1;
        *deadCount -= 1;
    }
    //If max enemies have spawned
    if (*deadCount > MAX_ENEMIES/2 && *aliveCount == MAX_ENEMIES)    //Waits for 6 or more enemies to die, before spawning again
        *aliveCount = MAX_ENEMIES - *deadCount;
}

//Custom function to handle enemy AI behavior
void handleEnemyAI(entityData *enemy, entityData player, Rectangle *inUseEnvironmentHitbox) {
    float hypotenuse, opposite, adjacent, oppositeAbs, adjacentAbs;
    //Type 1 enemies AI
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemy[i].type != 1) //If enemy has type 1
            continue;
        //Will follow the player aggressively if hit
        if (enemy[i].health < enemy[i].maxHealth) { 
            if (enemy[i].x + enemy[i].width > player.x && enemy[i].x < player.x + player.width)
                enemy[i].speed_x = 0;
            else if (enemy[i].x >= player.x + player.width)
                enemy[i].speed_x = -3 - (enemy[i].maxHealth - enemy[i].health)/enemy[i].maxHealth*2;    //Speed increases as the player continues hitting
                else enemy[i].speed_x = 3 + (enemy[i].maxHealth - enemy[i].health)/enemy[i].maxHealth*2;
        } else {
            //If it wasn't hit by the player, will move above its spawning platform without falling at a slow speed
            if (enemy[i].x < enemy[i].hitboxStart || enemy[i].x + enemy[i].width > enemy[i].hitboxEnd)
                enemy[i].speed_x *= -1;    
        }
    } 
    //Type 2 enemies AI 
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemy[i].type != 2) //If enemy has type 2
            continue;  
        //If the enemy has reached the player, the enemy stops
        if (enemy[i].x + enemy[i].width/2 > player.x && enemy[i].x + enemy[i].width/2 < player.x + player.width)
            if (enemy[i].y + enemy[i].height/2 > player.y && enemy[i].y + enemy[i].height/2 < player.y + player.height) {
                enemy[i].speed_x = 0;
                enemy[i].speed_y = 0;
                continue;
            }
        //If the enemy hasn't reached the player, the enemy follows it
        //Calculates distance from player using a right triangle
        oppositeAbs = opposite = player.x + player.width/2 - enemy[i].x - enemy[i].width/2;         
        adjacentAbs = adjacent = player.y + player.height/2 - enemy[i].y - enemy[i].height/2;
        hypotenuse = sqrt(pow(opposite, 2) + pow(adjacent, 2));
        //Calculates absolute value for the opposite and adjacent sides
        if (opposite < 0)       
            oppositeAbs *= -1;
        if (adjacent < 0)
            adjacentAbs *= -1;
        //Determines X and Y speeds
        if (oppositeAbs <= adjacentAbs) {               //If X distance is smaller than Y distance
            enemy[i].speed_y = 2;                       //Caps Y speed at 2 px/frame
            if (adjacent < 0)   
                enemy[i].speed_y *= -1;                 //Inverts Y movement direction if below the player
            enemy[i].speed_x = (opposite/hypotenuse)*2; //X speed is equal to the sin of theta times 2 px/frame 
        } else {                                        //If Y distance is smaller than X distance
            enemy[i].speed_x = 2;                       //Caps X speed at 2 px/frame          
            if (opposite < 0)
                enemy[i].speed_x *= -1;                 //Inverts X movement direction if to the right of the player
            enemy[i].speed_y = (adjacent/hypotenuse)*2; //Y speed is equal to the cos of theta times 2 px/frame
        }
    } 
}

//Custom function to reset player stats
void resetPlayerStats(playerStats *playerStatsHUD) {
    playerStatsHUD->baseDamage = BASE_DAMAGE;
    playerStatsHUD->baseHealthRegen = BASE_HEALTH_REGEN;
    playerStatsHUD->baseLuck = BASE_LUCK;
    playerStatsHUD->baseSpeed = BASE_SPEED;
}

//Custom function to handle item effects
void handleItemEffect(playerStats *playerStatsHUD, bool playerOnChest) {
    //Reset player stats when no item is active
    if (playerStatsHUD->itemActive == -1)
        resetPlayerStats(playerStatsHUD);
    //Apply item effect if there's one equipped and the player pressed "E" while not on a chest
    if (playerStatsHUD->itemEquipped != -1 && IsKeyPressed(KEY_E) && playerOnChest == false) {
        playerStatsHUD->itemActive = playerStatsHUD->itemEquipped;  //Activate item
        playerStatsHUD->itemEquipped = -1;                          //Unequip item
        resetPlayerStats(playerStatsHUD);                           //Reset player stats (cancel any active effects)
        switch (playerStatsHUD->itemActive) {                       //Apply item effect and timer
        case 1:     //Midas Touch
            playerStatsHUD->baseLuck = BASE_LUCK*3, playerStatsHUD->itemTimer = playerStatsHUD->elapsedTime + 5;
            break;
        case 2:     //Artemis Fierce
            playerStatsHUD->baseDamage = BASE_DAMAGE*2, playerStatsHUD->itemTimer = playerStatsHUD->elapsedTime + 10;
            break;
        case 3:     //Hermes Boots
            playerStatsHUD->baseSpeed = BASE_SPEED*1.6, playerStatsHUD->itemTimer = playerStatsHUD->elapsedTime + 15;
            break;
        }
    }
}

//Custom function to randomize the initial map
void randomizeInitialMap(int *mapLeftScreen, int *mapMiddleScreen, int *mapRightScreen, int *mapScreenOffsetX) {
    *mapLeftScreen = GetRandomValue(0, 4);          //Left screen

    do { *mapMiddleScreen = GetRandomValue(0, 4);   //Middle screen
    } while (*mapMiddleScreen == *mapLeftScreen);
    
    do { *mapRightScreen = GetRandomValue(0, 4);    //Right screen
    } while (*mapRightScreen == *mapLeftScreen || *mapRightScreen == *mapMiddleScreen);

    //Initialize screen offsets
    mapScreenOffsetX[*mapLeftScreen] = -1280;
    mapScreenOffsetX[*mapMiddleScreen] = 0;
    mapScreenOffsetX[*mapRightScreen] = 1280;
}

bool gameoverScreen(overlayVariables *overlayData, sharedOptions *gameOptions, playerStats *playerStatsHUD, Sound playingMusic, Texture2D currentBg, Font pixellariFont) {
    int count = 0, maxRGB = 255, step = 5;              //Counter variable for animation loops and animation presets
    unsigned char red = maxRGB, green = 0, blue = 0;    //RGB animations variables
    int minutes, seconds, totalScore;                   //Variables used in the score calculation
    bool newHighscore = false;
    //Game over screen
    while (!WindowShouldClose()) {
        //Background Music
        if (IsSoundPlaying(playingMusic) == false)  //Loop bg music
            PlaySound(playingMusic);
        if (IsSoundPlaying(playingMusic) == true)   //Change bg music volume  
            SetSoundVolume(playingMusic, (float)gameOptions->musicVolume * 1 / 100);

        //Draw fixed info
        BeginDrawing();
        ClearBackground(GRAY);  //Clear background
        DrawTexture(currentBg, 0, 0, (Color){100, 100, 100, 255});  //Background (darker gameplay screen)
        DrawTextExCenterHover(pixellariFont, "GAME OVER", (Vector2){SCREEN_WIDTH/2, 0.15*SCREEN_HEIGHT}, 45, 0, WHITE, WHITE); //GAME OVER

        //Scores
        //Time in mm : ss
        minutes = playerStatsHUD->elapsedTime/60;
        seconds = playerStatsHUD->elapsedTime - minutes*60;
    
        DrawTextEx(pixellariFont, TextFormat("TIME:"), (Vector2){410, 230}, 30, 0, WHITE);                              //TIME
        DrawTextEx(pixellariFont, TextFormat("%02d : %02d", minutes, seconds), (Vector2){700, 230}, 30, 0, WHITE);      //TIME
        DrawTextEx(pixellariFont, TextFormat("%d", playerStatsHUD->elapsedTime*2), (Vector2){840, 230}, 30, 0, YELLOW); //TIME

        DrawTextEx(pixellariFont, TextFormat("LEVEL:", playerStatsHUD->level), (Vector2){410, 230 + 50}, 30, 0, WHITE); //LEVEL
        DrawTextEx(pixellariFont, TextFormat("%d", playerStatsHUD->level), (Vector2){700, 230 + 50}, 30, 0, WHITE);     //LEVEL
        DrawTextEx(pixellariFont, TextFormat("%d", playerStatsHUD->level*5), (Vector2){840, 230 + 50}, 30, 0, YELLOW);  //LEVEL

        DrawTextEx(pixellariFont, TextFormat("ENEMIES KILLED:", playerStatsHUD->enemiesKilled), (Vector2){410, 230 + 50*2}, 30, 0, WHITE);  //ENEMIES
        DrawTextEx(pixellariFont, TextFormat("%d", playerStatsHUD->enemiesKilled), (Vector2){700, 230 + 50*2}, 30, 0, WHITE);               //ENEMIES
        DrawTextEx(pixellariFont, TextFormat("%d", playerStatsHUD->enemiesKilled*10), (Vector2){840, 230 + 50*2}, 30, 0, YELLOW);           //ENEMIES

        DrawTextEx(pixellariFont, TextFormat("TOTAL GOLD:", playerStatsHUD->totalGold), (Vector2){410, 230 + 50*3}, 30, 0, WHITE);  //GOLD
        DrawTextEx(pixellariFont, TextFormat("%d", playerStatsHUD->totalGold), (Vector2){700, 230 + 50*3}, 30, 0, WHITE);           //GOLD
        DrawTextEx(pixellariFont, TextFormat("%d", playerStatsHUD->totalGold*5), (Vector2){840, 230 + 50*3}, 30, 0, YELLOW);        //GOLD

        totalScore = playerStatsHUD->elapsedTime*2 + playerStatsHUD->level*5 + playerStatsHUD->enemiesKilled*10 + playerStatsHUD->totalGold*5;
        DrawTextEx(pixellariFont, TextFormat("FINAL SCORE:"), (Vector2){410, 230 + 50*4}, 30, 0, WHITE);    //SCORE
        DrawTextEx(pixellariFont, TextFormat("%d", totalScore), (Vector2){840, 230 + 50*4}, 30, 0, YELLOW); //SCORE

        //New highscore
        if (totalScore > gameOptions->highscore) {
            //Draw new highscore and its RGB effects
            count++;   
            if (count > 60) count = 0;
            //RGB First cycle
            if (red == maxRGB && blue == 0 && green != maxRGB) 
                green+=step;
            if (green == maxRGB && blue == 0 && red != 0) 
                red-=step;
            //RGB Second cycle
            if (red == 0 && green == maxRGB && blue != maxRGB) 
                blue+=step;
            if (blue == maxRGB && red == 0 && green != 0) 
                green-=step;
            //RGB Third cycle
            if (blue == maxRGB && green == 0 && red != maxRGB) 
                red+=step;
            if (red == maxRGB && green == 0 && blue != 0) 
                blue-=step;
            DrawTextExCenterHover(pixellariFont, "NEW HIGHSCORE", (Vector2){SCREEN_WIDTH/2, 515}, 30, 0, (Color){red, green, blue, 255}, (Color){red, green, blue, 255});
            newHighscore = true;
        }

        drawHighscore(gameOptions, pixellariFont);  //Current highscore
        //Return to title screen        
        if (DrawTextExCenterHover(pixellariFont, "TO TITLE SCREEN", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 230}, 35, 0, WHITE, YELLOW) || IsKeyPressed(KEY_ESCAPE)) {
            StopSound(playingMusic);
            if (newHighscore == true) {     //Save highscore (if set) on current save
                gameOptions->highscore = totalScore;
                writeSave(gameOptions->currentSaveSlot, gameOptions);
                sendOverlayInfo("Highscore saved", overlayData);
            }
            return false;   //Game over screen ended because the user clicked on "TO TITLE SCREEN"
        }  
        
        //Draw any overlay info that was on queue
        drawOverlayInfo(pixellariFont, overlayData);    
        EndDrawing(); 
    }    
    return true;    //Game over screen ended because the user closed the window
}

int titleScreenSettings(bool inGame, overlayVariables *overlayData, sharedOptions *gameOptions, Sound playingMusic, Texture2D currentBg, Font pixellariFont) {
    //Load Textures/Images
    Texture2D eraseSaveTrash = LoadTexture("images/icons/erase_save_trash.png");
    while (!WindowShouldClose()) {
        //Background Music
        if (IsSoundPlaying(playingMusic) == false)  //Loop bg music
            PlaySound(playingMusic);
        if (IsSoundPlaying(playingMusic) == true)   //Change bg music volume  
            SetSoundVolume(playingMusic, (float)gameOptions->musicVolume * 1 / 100);

        //Draw fixed info
        BeginDrawing(); 
        if (inGame) 
            DrawTexture(currentBg, 0, 0, (Color){100, 100, 100, 255});  //Background (if in game, the background is a darker gameplay screen)
        else 
            DrawTexture(currentBg, 0, 0, WHITE); 
        DrawTextExCenterHover(pixellariFont, "SETTINGS", (Vector2){SCREEN_WIDTH/2, 0.15*SCREEN_HEIGHT}, 45, 0, WHITE, WHITE); //SETTINGS
        DrawTextExCenterHover(pixellariFont, "RISK OF PAIN v.0.1 by Gab", (Vector2){SCREEN_WIDTH/2, 0.98*SCREEN_HEIGHT}, 15, 0, WHITE, WHITE);    //Credits
        drawHighscore(gameOptions, pixellariFont);  //Current highscore
        
        //Music controls
        DrawTextEx(pixellariFont, "MUSIC VOLUME", (Vector2){490, 220}, 20, 0, WHITE);
        DrawRectangleV((Vector2){690, 220}, (Vector2){100, 16}, (Color){255, 255, 255, 80});    //Transparent slider
        //Change music volume and draw the slider in different positions when user tweaks the volume
        Color sliderPointerColor = WHITE; 
        int musicVolumeSliderPos = gameOptions->musicVolume;                                    
        Vector2 currentMousePosition = GetMousePosition();  
        if (currentMousePosition.x >= 690 && currentMousePosition.x <= 790) {                   //Is mouse hovering and pressing
            if (currentMousePosition.y >= 210 && currentMousePosition.y <= 250) { 
                sliderPointerColor = YELLOW;    
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) == true)     
                    musicVolumeSliderPos = currentMousePosition.x - 690;                        //Updates volume
            } else sliderPointerColor = WHITE; 
        } else sliderPointerColor = WHITE;
        gameOptions->musicVolume = musicVolumeSliderPos;
        DrawRectangleV((Vector2){690, 220}, (Vector2){gameOptions->musicVolume, 16}, WHITE);    //Opaque slider   
        DrawRectangleV((Vector2){690 + gameOptions->musicVolume - 7, 220 - 6}, (Vector2){14, 28}, sliderPointerColor);   //Slider position indicator

        //SFX controls
        DrawTextEx(pixellariFont, "SFX VOLUME", (Vector2){490, 280}, 20, 0, WHITE);
        DrawRectangleV((Vector2){690, 280}, (Vector2){100, 16}, (Color){255, 255, 255, 80});    //Transparent slider
        //Change sfx volume and draw the slider in different positions when user tweaks the volume
        int sfxVolumeSliderPos = gameOptions->sfxVolume;   
        sliderPointerColor = WHITE;                                 
        currentMousePosition = GetMousePosition();  
        if (currentMousePosition.x >= 690 && currentMousePosition.x <= 790) {                   //Is mouse hovering and pressing
            if (currentMousePosition.y >= 270 && currentMousePosition.y <= 310) {
                sliderPointerColor = YELLOW;    
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) == true)       
                    sfxVolumeSliderPos = currentMousePosition.x - 690;                          //Updates volume
            } else sliderPointerColor = WHITE;               
        } else sliderPointerColor = WHITE;    
        gameOptions->sfxVolume = sfxVolumeSliderPos;
        DrawRectangleV((Vector2){690, 280}, (Vector2){gameOptions->sfxVolume, 16}, WHITE);      //Opaque slider 
        DrawRectangleV((Vector2){690 + gameOptions->sfxVolume - 14/2, 280 - 6}, (Vector2){14, 28}, sliderPointerColor);   //Slider position indicator
        
        //Save game select and erase (cannot be changed while in game)
        if (!inGame) { 
            DrawTextEx(pixellariFont, "SELECT SAVE SLOT", (Vector2){490, 340}, 20, 0, WHITE);
            DrawRectangleLinesEx((Rectangle){690, 337, 22, 22}, 3, WHITE);  //Outlines
            DrawRectangleLinesEx((Rectangle){729, 337, 22, 22}, 3, WHITE); 
            DrawRectangleLinesEx((Rectangle){768, 337, 22, 22}, 3, WHITE);
            //Select save slot
            for(int i = 0, j = 1; i <= 78 && j <= 3; i += 39, j++) {        //Change and load save
                if (checkMouseHoverRectangle((Rectangle){690 + i, 337, 22, 22}) == true)
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
                        if (j != gameOptions->currentSaveSlot)              //Will change save only if the selected save is different from the current one
                            readSave(j, gameOptions, overlayData);
            }
            DrawRectangleV((Vector2){656 + gameOptions->currentSaveSlot*39, 342}, (Vector2){12, 12}, YELLOW); //Selected save
            //Erase save slot
            Color trashColor;
            if (checkMouseHoverRectangle((Rectangle){807, 337, 22, 22}) == true) {      //Check mouse hover over the trash
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) == true) {                  //If pressed
                    createNewSave(gameOptions->currentSaveSlot, true, overlayData);     //Erase save (creates a new one with default values and a new world seed)
                    readSave(gameOptions->currentSaveSlot, gameOptions, overlayData);   //Reload save
                    //Send info to overlay on the screen
                    char overlay[100], id[2];
                    strcpy(overlay, "Save ");
                    sprintf(id, "%d", gameOptions->currentSaveSlot);
                    strcat(overlay, id);
                    strcat(overlay, " erased\nA new one has been created");
                    sendOverlayInfo(overlay, overlayData);
                }
                trashColor = YELLOW;
            } else trashColor = WHITE;
            DrawTexture(eraseSaveTrash, 807, 337, trashColor); //Trash icon 22 x 22 pixels 
        } else {
            //Cannot change/erase save while in game
            DrawTextEx(pixellariFont, "CAN ONLY CHANGE/ERASE SAVE\n         FROM TITLE SCREEN", (Vector2){490, 340}, 20, 0, WHITE); 
            //BACK TO TITLE SCREEN
            if (DrawTextExCenterHover(pixellariFont, "TO TITLE SCREEN", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 180}, 35, 0, WHITE, YELLOW) == true) {
                UnloadTexture(eraseSaveTrash);
                StopSound(playingMusic);
                writeSave(gameOptions->currentSaveSlot, gameOptions);   //Save settings on current save
                sendOverlayInfo("Settings saved", overlayData);
                return 2;    //Settings screen ended because the user clicked on "TO TITLE SCREEN"
            }  
        }

        //BACK
        if (DrawTextExCenterHover(pixellariFont, "BACK", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 230}, 35, 0, WHITE, YELLOW) == true || IsKeyPressed(KEY_ESCAPE)) {
            UnloadTexture(eraseSaveTrash);
            writeSave(gameOptions->currentSaveSlot, gameOptions);   //Save settings on current save
            sendOverlayInfo("Settings saved", overlayData);
            return false;    //Settings screen ended because the user clicked on "BACK"
        }
        //Draw any overlay info that was on queue
        drawOverlayInfo(pixellariFont, overlayData);    
        EndDrawing(); 
    }    
    UnloadTexture(eraseSaveTrash);
    return true;   //Settings screen ended because the user closed the window 
}

//Custom function to unload everything the gameplay screen used
void unloadEverything(Texture2D *mainHUD, Texture2D *skillHUD, Texture2D *simpleIcons, Texture2D *background, Texture2D *playerWalk, Texture2D *mapScreen, Texture2D *enemy2D, Sound *gameplayMusic, Sound *sfx) {
    //Unload Sounds
    UnloadSound(*gameplayMusic);
    for (int i = 0; i < 8; i++)
        UnloadSound(sfx[i]);
    //Unload Textures
    for (int i = 0; i < 4; i++)
        UnloadTexture(mainHUD[i]);
    for (int i = 0; i < 9; i++)
        UnloadTexture(skillHUD[i]);
    for (int i = 0; i < 6; i++)
        UnloadTexture(simpleIcons[i]);
    for (int i = 0; i < 2; i++)
        UnloadTexture(background[i]);
    for (int i = 0; i < 16; i++)
        UnloadTexture(playerWalk[i]);
    for (int i = 0; i < 5; i++)
        UnloadTexture(mapScreen[i]);
    for (int i = 0; i < 6; i++)
        UnloadTexture(enemy2D[i]);
}

//Custom function to load everything the gameplay screen needs
void loadEverything(Texture2D *mainHUD, Texture2D *skillHUD, Texture2D *simpleIcons, Texture2D *background, Texture2D *playerWalk, Texture2D *mapScreen, Texture2D *enemy2D, Sound *gameplayMusic, Sound *sfx) {
    //HUD, effects, background and some icons
    mainHUD[0] = LoadTexture("images/icons/main_hud_back.png");
    mainHUD[1] = LoadTexture("images/icons/main_hud_front.png");
    mainHUD[2] = LoadTexture("images/icons/main_hud_item_background.png");
    mainHUD[3] = LoadTexture("images/icons/quick_settings.png");
    skillHUD[0] = LoadTexture("images/icons/skill_1.png");
    skillHUD[1] = LoadTexture("images/icons/skill_2.png");
    skillHUD[2] = LoadTexture("images/icons/skill_3.png");
    skillHUD[3] = LoadTexture("images/icons/item_1.png");
    skillHUD[4] = LoadTexture("images/icons/item_2.png");
    skillHUD[5] = LoadTexture("images/icons/item_3.png");
    skillHUD[6] = LoadTexture("images/icons/item_1_info.png");
    skillHUD[7] = LoadTexture("images/icons/item_2_info.png");
    skillHUD[8] = LoadTexture("images/icons/item_3_info.png");
    simpleIcons[0] = LoadTexture("images/icons/simple_shot.png");
    simpleIcons[1] = LoadTexture("images/icons/simple_shot_left.png");
    simpleIcons[2] = LoadTexture("images/icons/coin.png");
    simpleIcons[3] = LoadTexture("images/icons/chest_locked.png");
    simpleIcons[4] = LoadTexture("images/icons/chest_open.png");
    simpleIcons[5] = LoadTexture("images/icons/coin_chest.png");
    background[0] = LoadTexture("images/background/gameplay_bg.png");
    background[1] = LoadTexture("images/background/gameplay_bg_2.png");
    //Player walk animation
    playerWalk[0] = LoadTexture("images/environment/player_idle.png");          //0, 2, 4, 6, 8, 10, 12 Right walking animation
    playerWalk[1] = LoadTexture("images/environment/player_idle_left.png");     //1, 3, 5, 7, 9, 11, 13 Left walking animation
    playerWalk[2] = LoadTexture("images/environment/player_walk_1.png");
    playerWalk[3] = LoadTexture("images/environment/player_walk_1_left.png");
    playerWalk[4] = LoadTexture("images/environment/player_walk_2.png");
    playerWalk[5] = LoadTexture("images/environment/player_walk_2_left.png");
    playerWalk[6] = LoadTexture("images/environment/player_walk_3.png");
    playerWalk[7] = LoadTexture("images/environment/player_walk_3_left.png");
    playerWalk[8] = LoadTexture("images/environment/player_walk_4.png");
    playerWalk[9] = LoadTexture("images/environment/player_walk_4_left.png");
    playerWalk[10] = LoadTexture("images/environment/player_walk_5.png");
    playerWalk[11] = LoadTexture("images/environment/player_walk_5_left.png");
    playerWalk[12] = LoadTexture("images/environment/player_walk_6.png");
    playerWalk[13] = LoadTexture("images/environment/player_walk_6_left.png");
    playerWalk[14] = LoadTexture("images/environment/player_dash.png");
    playerWalk[15] = LoadTexture("images/environment/player_dash_left.png");
    //Map screens
    mapScreen[0] = LoadTexture("images/background/platform_1.png");
    mapScreen[1] = LoadTexture("images/background/platform_2.png");
    mapScreen[2] = LoadTexture("images/background/platform_3.png");
    mapScreen[3] = LoadTexture("images/background/platform_4.png");
    mapScreen[4] = LoadTexture("images/background/platform_5.png");
    //Enemies
    enemy2D[0] = LoadTexture("images/environment/alien_1.png");
    enemy2D[1] = LoadTexture("images/environment/alien_2.png");
    enemy2D[2] = LoadTexture("images/environment/alien_3.png");
    enemy2D[3] = LoadTexture("images/environment/alien_4.png");
    enemy2D[4] = LoadTexture("images/environment/robot_1.png");
    enemy2D[5] = LoadTexture("images/environment/robot_2.png");
    //Gameplay music
    *gameplayMusic = LoadSound("sounds/soundtrack/gameplay_1.wav");
    //Sound effects (SFX)
    sfx[0] = LoadSound("sounds/sfx/laser.wav");
    sfx[1] = LoadSound("sounds/sfx/gunshot.wav");
    sfx[2] = LoadSound("sounds/sfx/shell.wav");
    sfx[3] = LoadSound("sounds/sfx/dash.wav");
    sfx[4] = LoadSound("sounds/sfx/death.wav");
    sfx[5] = LoadSound("sounds/sfx/player_hit.wav");
    sfx[6] = LoadSound("sounds/sfx/coin.wav");
    sfx[7] = LoadSound("sounds/sfx/level_up.wav");
}

bool gameplayScreen(overlayVariables *overlayData, sharedOptions *gameOptions, Font pixellariFont) {
    //Loading Screen
    Texture2D titleScreenBg = LoadTexture("images/background/title_screen_bg.png");
    DrawTexture(titleScreenBg, 0, 0, WHITE);
    DrawTextExCenterHover(pixellariFont, "LOADING...", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 30}, 35, 0, WHITE, WHITE);
    drawOverlayInfo(pixellariFont, overlayData);
    EndDrawing();
    UnloadTexture(titleScreenBg);

    //2D Camera initialization
    Camera2D camera = {(Vector2){0, 0}, (Vector2){0, 0}, 0, 1};
    
    //General variables
    playerStats playerStatsHUD = {BASE_LUCK, BASE_SPEED, BASE_DAMAGE, BASE_HEALTH_REGEN, 100, 100, 100, 0, 80, 1, 0, 0, 0, 0, 0, 0, -1, -1, 0};    //Player info on HUD
    int frameCount = 0;                                         //Frame counter
    Rectangle inUseEnvironmentHitbox[30];                       //Array of rectangles defining all in use environment hitboxes (walls/platforms/ground)
    int jumpTimer = 0, environmentHitboxCount = 0;              //Jump timer and quantity of environment hitboxes in use (up to 30)
    bool isPlayerOnGround = false, canPlayerJump = false, isDashing = false, isLaserActive = false;         //Player physics flags
    int shotTimer = 0, dashTimer = 0, laserTimer = 0;           //Timer for skills 1-3
    int closestEnemy;                                           //Used for the simple shot skill
    float parallaxOffsetMountainX = 0, parallaxOffsetSkyX = 0, newOffsetMountainX = 0, newOffsetSkyX = 0;   //Offset for the parallax effect on both mountain and sky
    float playerMiddleScreenOffsetX = 0;                        //Player offset from the middle screen (used in map generation)
    int walkAnimCount = 1;                                      //Walk animation frame count   
    int levelUpAnim = 0;  
    bool walkLeft = false;                                      //Flag to invert the player walking animation
    bool playerOnChest = false;

    //Player and enemy variables
    entityData player = {0, 600, 400, 27, 50, 0, 0};            //600, 500 position; 27, 50 size; 0, 0 speeds
    int lastDamageTaken = 0;                      
    entityData enemy[MAX_ENEMIES];
    for (int i = 0; i < MAX_ENEMIES; i++)
        enemy[i].type = -1;
    int aliveCount = 0;
    int deadCount = MAX_ENEMIES;
    int lastEnemySpawn = 0;

    //Randomize initial map screens
    int mapLeftScreen, mapMiddleScreen, mapRightScreen;
    int mapScreenOffsetX[5] = {-1, -1, -1, -1, -1};
    randomizeInitialMap(&mapLeftScreen, &mapMiddleScreen, &mapRightScreen, mapScreenOffsetX);

    //Chests and coins
    entityData coin[MAX_COINS];     //Up to 256 coins on the screen
    for (int i = 0; i < MAX_COINS; i++)     //Set type to "not used"
        coin[i].type = -1;
    entityData chest[MAX_CHESTS];   //Up to 2 chests on the screen
    for (int i = 0; i < MAX_CHESTS; i++)    //Set type to "not used"
        chest[i].type = -1;

    //Load Everything
    Texture2D mainHUD[4];
    Texture2D skillHUD[9];
    Texture2D simpleIcons[6];
    Texture2D background[2];
    Sound gameplayMusic;
    Texture2D playerWalk[16];
    Texture2D mapScreen[5];
    Texture2D enemy2D[6];
    Sound sfx[8];
    loadEverything(mainHUD, skillHUD, simpleIcons, background, playerWalk, mapScreen, enemy2D, &gameplayMusic, sfx);
    
    //Main gameplay loop
    while (!WindowShouldClose()) {
        //BACKGROUND MUSIC
        if (IsSoundPlaying(gameplayMusic) == false)  //Loop bg music
            PlaySound(gameplayMusic);
        if (IsSoundPlaying(gameplayMusic) == true)   //Change bg music volume  
            SetSoundVolume(gameplayMusic, (float)gameOptions->musicVolume * 1 / 100);

        //DRAWING EVERYTHING
        BeginDrawing();
        ClearBackground(GRAY);  //Clear background
        //From here on out every drawing will move with the camera
        BeginMode2D(camera);    

        //Update X offsets according to player X speed (used in parallax and map generation)
        playerMiddleScreenOffsetX += player.speed_x;
        parallaxOffsetMountainX += player.speed_x;
        parallaxOffsetSkyX += player.speed_x;
        
        //DRAW PARALLAX BACKGROUND
        drawParallaxBackground(&parallaxOffsetMountainX, &newOffsetMountainX, &parallaxOffsetSkyX, &newOffsetSkyX, player, background);

        //MAP GENERATION AND DRAWING
        generateMap(&mapLeftScreen, &mapMiddleScreen, &mapRightScreen, mapScreen, mapScreenOffsetX, inUseEnvironmentHitbox, &environmentHitboxCount, &playerMiddleScreenOffsetX);

        //SPAWN ENEMIES AND CHESTS
        spawnEnemies(enemy, &aliveCount, &deadCount, &lastEnemySpawn, player, inUseEnvironmentHitbox, environmentHitboxCount, playerStatsHUD);
        spawnChests(chest, player, inUseEnvironmentHitbox, environmentHitboxCount, playerStatsHUD);

        //ENEMY AI
        handleEnemyAI(enemy, player, inUseEnvironmentHitbox);

        //PLAYER INPUT
        //Use items
        handleItemEffect(&playerStatsHUD, playerOnChest);

        //Left and Right movement
        player.speed_x = 0;
        if (IsKeyDown(KEY_A))
            player.speed_x = -playerStatsHUD.baseSpeed;
        if (IsKeyDown(KEY_D))
            player.speed_x = playerStatsHUD.baseSpeed;
        //Skills
        if (IsKeyPressed(KEY_H))    //Skill (Simple shot)
            if (shotTimer == 0) {
                shotTimer = 1;
                PlaySound(sfx[1]);
                PlaySound(sfx[2]);
            }
        if (IsKeyPressed(KEY_J))    //Skill 2 (Laser)
            if(playerStatsHUD.CooldownSkill2 == 0) {
                playerStatsHUD.CooldownSkill2 = playerStatsHUD.elapsedTime + 3;
                isLaserActive = true;
                PlaySound(sfx[0]);
            }
        if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_LEFT_SHIFT))    //Skill 3 (Dash)
            if(playerStatsHUD.CooldownSkill3 == 0) {
                playerStatsHUD.CooldownSkill3 = playerStatsHUD.elapsedTime + 4;
                isDashing = true;
                PlaySound(sfx[3]);
            }

        //PHYSICS
        //Player Jump
        if (IsKeyDown(KEY_SPACE) && canPlayerJump == true) { 
            jumpTimer++;                //Jump timer, determines how high the player can jump while holding SPACE
            player.speed_y = -8.6;      //Move the player upwards
            isPlayerOnGround = false;   //isPlayerOnGround flag is set (cannot jump again while on the air)
        }
        if (IsKeyUp(KEY_SPACE) == true && isPlayerOnGround == true) {   //Can only jump again when the player hits the ground and SPACE is released
            canPlayerJump = true; 
            jumpTimer = 0;                                              //Reset jump timer  
        } 
        if (jumpTimer > 10 || (IsKeyUp(KEY_SPACE) == true && isPlayerOnGround == false)) //If the jump timer is above a threshold, cannot jump again
            canPlayerJump = false;                                                       //And also cannot jump again when falling after releasing SPACE early
        
        //Gravity, always active
        //(g = 0.6 pixels/frame or 36 pixels/second @ 60 fps)
            player.speed_y += 0.6; 
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (enemy[i].type == 1) //If an enemy is from type 1, applies gravity
                enemy[i].speed_y += 0.6;
        for (int i = 0; i < MAX_COINS; i++)
            if (coin[i].type == 1)  //If a coin is from type 1, applies gravity
                coin[i].speed_y += 0.6;
        
        //DASHING (SKILL 3)
        handleDash(&player, &dashTimer, &isDashing, walkLeft, playerWalk);
        
        //Player/environment collision detection v.2 (auto adjusts the player speed to prevent collision and detects when the player has hit the ground)
        isPlayerOnGround = false;
        handlePlayerEnvironmentCollision(inUseEnvironmentHitbox, environmentHitboxCount, &player, &isPlayerOnGround, &canPlayerJump);
        //Enemy/environment collision detection 
        handleEnemyEnvironmentCollision(inUseEnvironmentHitbox, environmentHitboxCount, enemy);
        //Coin/environment collision detection
        handleCoinEnvironmentCollision(inUseEnvironmentHitbox, environmentHitboxCount, coin);

        //DRAW CHESTS AND COINS
        drawChestCoins(coin, chest, simpleIcons, skillHUD, pixellariFont);

        //DRAW AND HANDLE PLAYER/ENEMIES/SKILLS
        drawPlayer(player, playerWalk, &walkLeft, &walkAnimCount);
        //Handle player damage and health regen
        handlePlayerDamage(sfx, player, &playerStatsHUD, enemy, &lastDamageTaken, isDashing);
        //Handle player coin/chest mechanics
        handlePlayerCoinChest(sfx, player, &playerStatsHUD, coin, chest, &playerOnChest, pixellariFont, overlayData);

        //DRAW AND HANDLE SIMPLE SHOT (SKILL 1)
        handleShot(&closestEnemy, &shotTimer, walkLeft, player, simpleIcons, playerStatsHUD, enemy);

        //DRAW AND HANDLE LASER (SKILL 2)
        handleLaser(&isLaserActive, &laserTimer, walkLeft, player, playerStatsHUD, enemy);

        //DRAW ENEMIES AND SPAWN COINS
        drawHandleEnemies(sfx, enemy, enemy2D, &deadCount, player, &playerStatsHUD, coin);

        //CAMERA
        camera = (Camera2D){(Vector2){600, 500}, (Vector2){player.x, player.y}, 0, 1};    
        //DrawRectangleLinesEx((Rectangle){-600 + player.x -player.speed_x,-500 + player.y -player.speed_y, 1280, 720}, 6, YELLOW);    //Camera screen DEBUG 
        EndMode2D();
        //Beyond this point every drawing will be fixed and won't move with the camera

        //PLAYER HUD
        //Elapsed time
        drawElapsedTime(&frameCount, &playerStatsHUD, pixellariFont);

        //Main HUD (Skills, gold, health, xp)
        bool hasDied = drawHUD(sfx, pixellariFont, mainHUD, skillHUD, &playerStatsHUD, player, lastDamageTaken, &levelUpAnim, playerOnChest);        

        //SFX VOLUME
        for (int i = 0; i < 8; i++) //Apply current sfx volume to all sounds playing
            SetSoundVolume(sfx[i], gameOptions->sfxVolume/100);   

        //Quick settings icon (last thing to be drawn because the pause menu needs to be generated when every action has already taken place)
        Color settingsIconColor = WHITE;
        if (checkMouseHoverRectangle((Rectangle){20, 20, 40, 40}))  //Change icon color if mouse is hovering
            settingsIconColor = YELLOW;        
        DrawTexture(mainHUD[3], 20+3, 20+3, (Color){0, 0, 0, 127});   //Drop shadow
        DrawTexture(mainHUD[3], 20, 20, settingsIconColor);

        if (hasDied == true) {                              //Game over screen
            drawOverlayInfo(pixellariFont, overlayData);    //Ends drawing
            EndDrawing();                                   //Needs 2 for some reason idk why 
            EndDrawing();
            Image screenCapture = LoadImageFromScreen();    //Capture current screen
            Texture2D gameplayScreenCapture = LoadTextureFromImage(screenCapture);  //Set current screen as a background for the game over screen
            bool exit = gameoverScreen(overlayData, gameOptions, &playerStatsHUD, gameplayMusic, gameplayScreenCapture, pixellariFont);
            unloadEverything(mainHUD, skillHUD, simpleIcons, background, playerWalk, mapScreen, enemy2D, &gameplayMusic, sfx);
            UnloadImage(screenCapture);
            UnloadTexture(gameplayScreenCapture);
            if (exit == true)
                return true;    //User closed the window
            else 
                return false;   //Return to title screen
        }

        //Quick settings screen (basically the pause menu)
        if ((checkMouseHoverRectangle((Rectangle){20, 20, 40, 40}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) { 
            drawOverlayInfo(pixellariFont, overlayData);    //Ends drawing
            EndDrawing();
            Image screenCapture = LoadImageFromScreen();    //Capture current screen
            Texture2D gameplayScreenCapture = LoadTextureFromImage(screenCapture);  //Set current screen as a background for the settings screen
            int exit = titleScreenSettings(true, overlayData, gameOptions, gameplayMusic, gameplayScreenCapture, pixellariFont);
            if (exit == true || exit == 2) {    //If the user closed the window or wants to go back to title screen, unload used textures/sounds
                unloadEverything(mainHUD, skillHUD, simpleIcons, background, playerWalk, mapScreen, enemy2D, &gameplayMusic, sfx);
                UnloadImage(screenCapture);
                UnloadTexture(gameplayScreenCapture);
            }
            if (exit == true)   //User closed the window
                return true;
            if (exit == 2)      //User wants to go back to title screen
                return false;
        }

        //Draw any overlay info that was on queue and the CRT effect    
        drawOverlayInfo(pixellariFont, overlayData);
        EndDrawing(); 
    }
    unloadEverything(mainHUD, skillHUD, simpleIcons, background, playerWalk, mapScreen, enemy2D, &gameplayMusic, sfx);
    return true;    //Gameplay screen ended because the user closed the window
}

bool titleScreenControls(overlayVariables *overlayData, sharedOptions *gameOptions, Sound titleScreenMusic, Texture2D titleScreenBg, Font pixellariFont) {
    //Load Textures/Images
    Texture2D controls = LoadTexture("images/icons/controls.png");
    while (!WindowShouldClose()) {
        //Background Music
        if (IsSoundPlaying(titleScreenMusic) == false)  //Loop bg music
            PlaySound(titleScreenMusic);
        if (IsSoundPlaying(titleScreenMusic) == true)   //Change bg music volume  
            SetSoundVolume(titleScreenMusic, (float)gameOptions->musicVolume * 1 / 100);

        //Draw fixed info
        BeginDrawing(); 
        ClearBackground(WHITE); 
        DrawTexture(titleScreenBg, 0, 0, WHITE);    //Background
        DrawTextExCenterHover(pixellariFont, "CONTROLS", (Vector2){SCREEN_WIDTH/2, 0.15*SCREEN_HEIGHT}, 45, 0, WHITE, WHITE); //Controls
        DrawTextExCenterHover(pixellariFont, "RISK OF PAIN v.0.1 by Gab", (Vector2){SCREEN_WIDTH/2, 0.98*SCREEN_HEIGHT}, 15, 0, WHITE, WHITE);    //Credits
        drawHighscore(gameOptions, pixellariFont);  //Current highscore

        //Controls
        DrawTexture(controls, SCREEN_WIDTH/2 -900/2, SCREEN_HEIGHT/2 -400/2 - 12, WHITE);
        
        //BACK
        if (DrawTextExCenterHover(pixellariFont, "BACK", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 230}, 35, 0, WHITE, YELLOW) == true || IsKeyPressed(KEY_ESCAPE)) {
            UnloadTexture(controls);
            return false;   //Controls screen ended because the user clicked on "BACK" or pressed "ESC"
        }
        drawOverlayInfo(pixellariFont, overlayData);        //Draw any overlay info that was on queue
        EndDrawing(); 
    }    
    UnloadTexture(controls);
    return true;   //Controls screen ended because the user closed the window 
}

void titleScreen(overlayVariables *overlayData, sharedOptions *gameOptions, Sound titleScreenMusic, Texture2D titleScreenBg, Font pixellariFont) {
    int count = 0, maxRGB = 255, step = 5;              //Counter variable for animation loops and animation presets
    unsigned char red = maxRGB, green = 0, blue = 0;    //RGB animations variables

    while (!WindowShouldClose()) {  
        //Background Music
        if (IsSoundPlaying(titleScreenMusic) == false)  //Loop bg music
            PlaySound(titleScreenMusic);
        if (IsSoundPlaying(titleScreenMusic) == true)   //Change bg music volume  
            SetSoundVolume(titleScreenMusic, (float)gameOptions->musicVolume * 1 / 100);

        //Draw fixed info
        BeginDrawing(); 
        ClearBackground(WHITE);
        DrawTexture(titleScreenBg, 0, 0, WHITE);    //Background
        DrawTextExCenterHover(pixellariFont, "RISK\n   PAIN", (Vector2){SCREEN_WIDTH/2, 0.22*SCREEN_HEIGHT}, 55, 0, WHITE, WHITE); //Title
        DrawTextExCenterHover(pixellariFont, "of", (Vector2){SCREEN_WIDTH/2, 0.215*SCREEN_HEIGHT}, 30, 0, WHITE, WHITE);
        DrawTextExCenterHover(pixellariFont, "RISK OF PAIN v.0.1 by Gab", (Vector2){SCREEN_WIDTH/2, 0.98*SCREEN_HEIGHT}, 15, 0, WHITE, WHITE);    //Credits
        drawHighscore(gameOptions, pixellariFont);  //Current highscore

        //Draw game name and its RGB effects
        count++;   
        if (count > 60) count = 0;
        //RGB First cycle
        if (red == maxRGB && blue == 0 && green != maxRGB) 
            green+=step;
        if (green == maxRGB && blue == 0 && red != 0) 
            red-=step;
        //RGB Second cycle
        if (red == 0 && green == maxRGB && blue != maxRGB) 
            blue+=step;
        if (blue == maxRGB && red == 0 && green != 0) 
            green-=step;
        //RGB Third cycle
        if (blue == maxRGB && green == 0 && red != maxRGB) 
            red+=step;
        if (red == maxRGB && green == 0 && blue != 0) 
            blue-=step;

        DrawRectangleLinesEx((Rectangle){SCREEN_WIDTH/2 - 186/2-12, 0.22*SCREEN_HEIGHT - 148/2-12, 216, 162}, 5, (Color){red, green, blue, 255}); //Fixed outline
        DrawRectangleLinesEx((Rectangle){SCREEN_WIDTH/2 - (186-count/2)/2-12, 0.22*SCREEN_HEIGHT - (148-count/2)/2-12, 216-count/2, 162-count/2}, count/10, (Color){red, green, blue, 255-count*4.25}); //Moving outline
        
        //Main title screen
        //PLAY
        if (DrawTextExCenterHover(pixellariFont, "PLAY", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 30}, 35, 0, WHITE, YELLOW) == true) {
            StopSound(titleScreenMusic);
            bool exit = gameplayScreen(overlayData, gameOptions, pixellariFont);  //Gameplay screen
            if (exit == true)
                return;     //User closed the window
        }    
            
        //Controls
        if (DrawTextExCenterHover(pixellariFont, "CONTROLS", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 80}, 35, 0, WHITE, YELLOW) == true) {
            bool exit = titleScreenControls(overlayData, gameOptions, titleScreenMusic, titleScreenBg, pixellariFont);    //Controls screen
            if (exit == true)
                return;     //User closed the window 
        } 
        
        //SETTINGS
        if (DrawTextExCenterHover(pixellariFont, "SETTINGS", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 130}, 35, 0, WHITE, YELLOW) == true) {
            bool exit = titleScreenSettings(false, overlayData, gameOptions, titleScreenMusic, titleScreenBg, pixellariFont);    //Options screen
            if (exit == true)
                return;     //User closed the window 
        }
        //Draw any overlay info that was on queue
        drawOverlayInfo(pixellariFont, overlayData);        
        EndDrawing();
        if (IsKeyPressed(KEY_ESCAPE))
            return;     //Title screen ended because the user pressed "ESC"
    }
    return;   //Title screen ended because the user closed the window
}

int main() {
    //Init
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "RISK OF PAIN v.0.1");
    InitAudioDevice();
    SetExitKey(KEY_NULL);   //Pressing ESC won't close the window by default

    //Target FPS
    SetTargetFPS(TARGET_FPS);  

    //Font to be used in the entire game
    Font pixellariFont = LoadFont("fonts/Pixellari.otf");

    //Load Bg Image
    Texture2D titleScreenBg = LoadTexture("images/background/title_screen_bg.png");

    //Load Sounds
    Sound titleScreenMusic = LoadSound("sounds/soundtrack/title_screen.mp3");  
    
    //Shared screen variables
    sharedOptions gameOptions;
    overlayVariables overlayData;

    //CRT effect initialization
    overlayData.scanlineActive = true;
    overlayData.scanlineTimer = 0;
    overlayData.scanlineStart = 0;
    overlayData.crtBorder = LoadTexture("images/background/crt_border.png");   

    //Saved games
    //Up to 3 saves
    createNewSave(1, false, &overlayData);
    createNewSave(2, false, &overlayData);
    createNewSave(3, false, &overlayData);

    //Loads default save (save 1)
    readSave(1, &gameOptions, &overlayData);

    //Title Screen
    //User can change some options while on title screen
    titleScreen(&overlayData, &gameOptions, titleScreenMusic, titleScreenBg, pixellariFont);

    //User closed the window (the game will close)
    //Unload everything
    UnloadFont(pixellariFont); 
    UnloadSound(titleScreenMusic);
    UnloadTexture(titleScreenBg);
    UnloadTexture(overlayData.crtBorder);

    //De-Init  
    CloseAudioDevice();
    CloseWindow();    

    return 0;
}