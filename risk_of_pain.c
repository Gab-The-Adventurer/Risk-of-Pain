//Orfanato Game Dev
//Risk of Pain by Gabriel Pains
#include "include/raylib.h"
#include "stdio.h"
#include "string.h"

//16:9 Screen Aspect Ratio and 60 fps
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TARGET_FPS 60

//Total number of hitboxes for a given screen 
#define platform_1_count 1      //Screen 0
#define platform_2_count 1      //Screen 1
#define platform_3_count 1      //Screen 2
#define platform_4_count 1      //Screen 3
#define platform_5_count 6      //Screen 4

//Game settings, shared between screens and the save games
typedef struct sharedOptions {
    int musicVolume;        //Music volume
    int sfxVolume;          //SFX volume
    int currentSaveSlot;    //Current save slot (1-3)
    int highscore;          //Player highscore, updated when going back to title screen or when dead
} sharedOptions;

//Player stats, used while in game
typedef struct playerStats {
    int health;
    int xp;
    int level;
    int gold;
    double elapsedTime;
} playerStats;

//Variables to be used with the overlay functions
typedef struct overlayVariables {
    bool drawing;
    char infoToDraw[100];
    double initialTime;
    int counter;
} overlayVariables;

//Custom function to overlay and animate any info on the screen automatically
void drawOverlayInfo(Font pixellariFont, overlayVariables *overlayData) {
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
}

//Custom function to send info to the drawOverlayInfo function
overlayVariables sendOverlayInfo(char *info) {  
    overlayVariables newOverlayData;            //Saves info in the struct overlayVariables
    newOverlayData.drawing = true;
    strcpy(newOverlayData.infoToDraw, info);
    newOverlayData.initialTime = GetTime();
    newOverlayData.counter = 0;
    return newOverlayData;
}

//Custom function that creates a new save file with default settings
void createNewSave(int saveID, bool overwrite, overlayVariables *overlayData) {
    //Default save settings
    sharedOptions newSaveData;
    newSaveData.musicVolume = 80;
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
    *overlayData = sendOverlayInfo(overlay);
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
    DrawTextEx(font, text, (Vector2){newPosition.x + 3, newPosition.y + 3}, fontSize, spacing, (Color){0, 0, 0, 127}); //Drop Shadow
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

//Custom function to handle player/environment collision, adjusts player speed to avoid collision and can set isPlayerOnGround and canPlayerJump flags
void handlePlayerEnvironmentCollision(Rectangle *environmentHitbox, int inUseEnvironmentHitboxes, Vector2 *playerPosition, Vector2 *playerSpeed, bool *isPlayerOnGround, bool *canPlayerJump) {
    Vector2 orgPlayerPosition = {playerPosition->x, playerPosition->y}, orgPlayerSpeed = {playerSpeed->x, playerSpeed->y}; //Original position and speed values are stored to be used when detecting collisions
    Vector2 newPlayerSpeed = orgPlayerSpeed;                //New player speed is initialized as equal to the original
    for (int i = 0; i < inUseEnvironmentHitboxes; i++) {    //For every in use environment hitbox, will test player/hitbox collisions
        //Check left and right collisions
        if(checkCollisionRectanglesLeft((Rectangle){orgPlayerPosition.x + orgPlayerSpeed.x, orgPlayerPosition.y, 30, 50}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
            newPlayerSpeed.x += environmentHitbox[i].x + environmentHitbox[i].width - orgPlayerPosition.x - orgPlayerSpeed.x;  //Update new player X speed if a collision was detected
        if(checkCollisionRectanglesRight((Rectangle){orgPlayerPosition.x + orgPlayerSpeed.x, orgPlayerPosition.y, 30, 50}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height}))
            newPlayerSpeed.x += environmentHitbox[i].x - orgPlayerPosition.x - 30 - orgPlayerSpeed.x;   //Update new player X speed if a collision was detected
        
        //Check top and bottom collisions
        if (checkCollisionRectanglesTop((Rectangle){orgPlayerPosition.x, orgPlayerPosition.y + orgPlayerSpeed.y, 30, 50}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height})) {
            newPlayerSpeed.y += environmentHitbox[i].y + environmentHitbox[i].height - orgPlayerPosition.y - orgPlayerSpeed.y; //Update new player Y speed if a collision was detected
            *canPlayerJump = false;     //If player collides top (head), resets jump, so the player needs to reach the ground before jumping again
        }
        if (checkCollisionRectanglesBottom((Rectangle){orgPlayerPosition.x, orgPlayerPosition.y + orgPlayerSpeed.y, 30, 50}, (Rectangle){environmentHitbox[i].x, environmentHitbox[i].y, environmentHitbox[i].width, environmentHitbox[i].height})) {
            newPlayerSpeed.y += environmentHitbox[i].y - orgPlayerPosition.y - 50 - orgPlayerSpeed.y; //Update new player Y speed if a collision was detected
            *isPlayerOnGround = true;   //If player collides bottom (feet), the player has reached the ground
        }      
    }
    playerPosition->x = orgPlayerPosition.x + newPlayerSpeed.x;     //Update player X position after changes
    playerPosition->y = orgPlayerPosition.y + newPlayerSpeed.y;     //Update player Y position after changes
    playerSpeed->x = newPlayerSpeed.x;      //Update player X speed after changes
    playerSpeed->y = newPlayerSpeed.y;      //Update player Y speed after changes
}

//Custom function to load any in use environment hitbox, changes when more map is generated
void loadEnvironmentHitboxes(Rectangle *inUseEnvironmentHitbox, int *inUseEnvironmentHitboxes, int *mapScreenOffsetX) {
    //Environment hitboxes lookup table
    //platform_1 screen hitboxes (Screen 0)   
    //This screen has 8 hitboxes and they are defined as follows:
    Rectangle platform_1_hitboxes[platform_1_count] = {{0 + mapScreenOffsetX[0] + 29, 490, 1280-29, 70}};    

    //platform_2 screen hitboxes (Screen 1)    
    //This screen has 8 hitboxes and they are defined as follows:
    Rectangle platform_2_hitboxes[platform_2_count] = {{0 + mapScreenOffsetX[1] + 29, 490, 1280-29, 70}}; 

    //platform_3 screen hitboxes (Screen 2)   
    //This screen has 8 hitboxes and they are defined as follows:
    Rectangle platform_3_hitboxes[platform_2_count] = {{0 + mapScreenOffsetX[2] + 29, 490, 1280-29, 70}}; 

    //platform_4 screen hitboxes (Screen 3)   
    //This screen has 8 hitboxes and they are defined as follows:
    Rectangle platform_4_hitboxes[platform_2_count] = {{0 + mapScreenOffsetX[3] + 29, 490, 1280-29, 70}}; 

    //platform_5 screen hitboxes (Screen 4)
    //This screen has 6 hitboxes and they are defined as follows:
    Rectangle platform_5_hitboxes[platform_5_count] = {
    {0 + mapScreenOffsetX[4], 550, 1280, 170}, {137 + mapScreenOffsetX[4], 442, 355, 30}, {824 + mapScreenOffsetX[4], 152, 455, 30},
    {1020 + mapScreenOffsetX[4], 490, 260, 70}, {576 + mapScreenOffsetX[4], 360, 385, 30}, {507 + mapScreenOffsetX[4], 231, 178, 30}};

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
    *inUseEnvironmentHitboxes = hitbox;
}

//Custom function that generates and draw new map screens as the player moves, also loads hitboxes from these screens to be used in the player/environment collision system
void generateMap(int *mapLeftScreen, int *mapMiddleScreen, int *mapRightScreen, Texture2D *mapScreen, int *mapScreenOffsetX, Rectangle *inUseEnvironmentHitbox, int *inUseEnvironmentHitboxes, float *playerMiddleScreenOffsetX) {
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
            DrawTexture(mapScreen[i], mapScreenOffsetX[i], -720, WHITE);
    //Load hitboxes for the given map
    loadEnvironmentHitboxes(inUseEnvironmentHitbox, inUseEnvironmentHitboxes, mapScreenOffsetX);
}

//Custom function to draw a mountain and a sky background with parallax effect
void drawParallaxBackground(float *parallaxOffsetMountainX, float *newOffsetMountainX, float *parallaxOffsetSkyX, float *newOffsetSkyX, float playerPositionY, Texture2D mountainBackground, Texture2D skyBackground) {
    //Divisions by 6 and 12 determines parallax speed/intensity
    if (*parallaxOffsetMountainX + *parallaxOffsetMountainX/6 >= 1280)                              //If player has traveled one screen to the right
        *newOffsetMountainX += 1280 - *parallaxOffsetMountainX/6, *parallaxOffsetMountainX = 0;     //Updates mountain position
    if (*parallaxOffsetMountainX + *parallaxOffsetMountainX/6 <= -1280)                             //If player has traveled one screen to the left
        *newOffsetMountainX -= 1280 + *parallaxOffsetMountainX/6, *parallaxOffsetMountainX = 0;     //Updates mountain position
    
    if (*parallaxOffsetSkyX + *parallaxOffsetSkyX/12 >= 1280)                           //If player has traveled one screen to the right
        *newOffsetSkyX += 1280 - *parallaxOffsetSkyX/12, *parallaxOffsetSkyX = 0;       //Updates sky position
    if (*parallaxOffsetSkyX + *parallaxOffsetSkyX/12 <= -1280)                          //If player has traveled one screen to the left
        *newOffsetSkyX -= 1280 + *parallaxOffsetSkyX/12, *parallaxOffsetSkyX = 0;       //Updates sky position

    DrawTexture(skyBackground, -1280 + *newOffsetSkyX - *parallaxOffsetSkyX/12, playerPositionY - 500, WHITE);                  //Draw sky
    DrawTexture(mountainBackground, -1280 + *newOffsetMountainX - *parallaxOffsetMountainX/6, playerPositionY - 500, WHITE);    //Draw mountain
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
        DrawTextExCenterHover(pixellariFont, "RISK OF PAIN v.0.1 by Gabriel Pains", (Vector2){SCREEN_WIDTH/2, 0.98*SCREEN_HEIGHT}, 15, 0, WHITE, WHITE);    //Credits
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
                    *overlayData = sendOverlayInfo(overlay);
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
                *overlayData = sendOverlayInfo("Settings saved");
                return 2;    //Settings screen ended because the user clicked on "TO TITLE SCREEN"
            }  
        }

        //BACK
        if (DrawTextExCenterHover(pixellariFont, "BACK", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 230}, 35, 0, WHITE, YELLOW) == true || IsKeyPressed(KEY_ESCAPE)) {
            UnloadTexture(eraseSaveTrash);
            writeSave(gameOptions->currentSaveSlot, gameOptions);   //Save settings on current save
            *overlayData = sendOverlayInfo("Settings saved");
            return false;    //Settings screen ended because the user clicked on "BACK"
        }
        //Draw any overlay info that was on queue (if the game is not paused)
        if (!inGame)
            drawOverlayInfo(pixellariFont, overlayData);    
        EndDrawing(); 
    }    
    UnloadTexture(eraseSaveTrash);
    return true;   //Settings screen ended because the user closed the window 
}

bool gameplayScreen(overlayVariables *overlayData, sharedOptions *gameOptions, Font pixellariFont) {
    //Loading Screen
    Texture2D titleScreenBg = LoadTexture("images/background/title_screen_bg.png");
    DrawTexture(titleScreenBg, 0, 0, WHITE);
    DrawTextExCenterHover(pixellariFont, "LOADING...", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 30}, 35, 0, WHITE, WHITE);
    EndDrawing();
    UnloadTexture(titleScreenBg);

    //2D Camera initialization
    Camera2D camera = {(Vector2){0, 0}, (Vector2){0, 0}, 0, 1};
    
    //Variables
    Vector2 playerPosition = {600, 500}, playerSpeed = {0, 0};  //Player starting position and speed
    Rectangle inUseEnvironmentHitbox[30];                       //Array of rectangles defining all in use environment hitboxes (walls/platforms/ground)
    int jumpTimer = 0, inUseEnvironmentHitboxes = 0;            //Jump timer and quantity of environment hitboxes in use (up to 30)
    bool isPlayerOnGround = false, canPlayerJump = false;       //Player physics flags
    float parallaxOffsetMountainX = 0, parallaxOffsetSkyX = 0, newOffsetMountainX = 0, newOffsetSkyX = 0;   //Offset for the parallax effect on both mountain and sky
    float playerMiddleScreenOffsetX = 0;                        //Player offset from the middle screen (used in map generation)

    int mapLeftScreen = 0, mapMiddleScreen = 4, mapRightScreen = 1;
    int mapScreenOffsetX[5] = {-1, -1, -1, -1, -1};
    mapScreenOffsetX[mapLeftScreen] = -1280;
    mapScreenOffsetX[mapMiddleScreen] = 0;
    mapScreenOffsetX[mapRightScreen] = 1280;

    //Load Textures/Images/Sounds
    Texture2D settingsIcon = LoadTexture("images/icons/quick_settings.png");
    Texture2D mountainBackground = LoadTexture("images/background/gameplay_bg.png");
    Texture2D skyBackground = LoadTexture("images/background/gameplay_bg_2.png");
    Sound gameplayMusic = LoadSound("sounds/soundtrack/player_normal.mp3"); 
    Sound gameplayMusic2 = LoadSound("sounds/soundtrack/gameplay_1.wav"); 

    //Map screens
    Texture2D mapScreen[5];
    mapScreen[0] = LoadTexture("images/background/platform_1.png");
    mapScreen[1] = LoadTexture("images/background/platform_2.png");
    mapScreen[2] = LoadTexture("images/background/platform_3.png");
    mapScreen[3] = LoadTexture("images/background/platform_4.png");
    mapScreen[4] = LoadTexture("images/background/platform_5.png");
    
    //Main gameplay loop
    while (!WindowShouldClose()) {
        //BACKGROUND MUSIC
        if (IsSoundPlaying(gameplayMusic2) == false)  //Loop bg music
            PlaySound(gameplayMusic2);
        if (IsSoundPlaying(gameplayMusic2) == true)   //Change bg music volume  
            SetSoundVolume(gameplayMusic2, (float)gameOptions->musicVolume * 1 / 100);

        BeginDrawing();
        ClearBackground(GRAY);  //Clear background
        //From here on out every drawing will move with the camera
        BeginMode2D(camera);    
        
        //DRAW PARALLAX BACKGROUND
        //Draw mountain and sky according to player + both mountain and sky X offsets
        drawParallaxBackground(&parallaxOffsetMountainX, &newOffsetMountainX, &parallaxOffsetSkyX, &newOffsetSkyX, playerPosition.y, mountainBackground, skyBackground);

        //MAP GENERATION AND DRAWING
        //Generates and draws new right and left screens if the player gets too far from the middle screen, 
        //also loads hitboxes from these screens to be used in the player/environment collision system
        generateMap(&mapLeftScreen, &mapMiddleScreen, &mapRightScreen, mapScreen, mapScreenOffsetX, inUseEnvironmentHitbox, &inUseEnvironmentHitboxes, &playerMiddleScreenOffsetX);

        //INPUT
        //Left and Right movement
        playerSpeed.x = 0;
        if (IsKeyDown(KEY_A))
            playerSpeed.x = -5;
        if (IsKeyDown(KEY_D))
            playerSpeed.x = 5;

        //PLAYER PHYSICS
        //Jump
        if (IsKeyDown(KEY_SPACE) && canPlayerJump == true) { 
            jumpTimer++;                //Jump timer, determines how high can the player jump while holding SPACE
            playerSpeed.y = -8.6;       //Move the player upwards
            isPlayerOnGround = false;   //isPlayerOnGround flag is set (gravity will start acting)
        }
        if (IsKeyUp(KEY_SPACE) == true && isPlayerOnGround == true) {   //Can only jump again when the player hits the ground and SPACE is released
            canPlayerJump = true; 
            jumpTimer = 0;                                              //Reset jump timer  
        } 
        if (jumpTimer > 10 || (IsKeyUp(KEY_SPACE) == true && isPlayerOnGround == false)) //If the jump timer is above a threshold, cannot jump again
            canPlayerJump = false;                                                       //And also cannot jump again when falling after releasing SPACE early
        
        //Gravity, always active
            playerSpeed.y += 0.6; //(g = 0.6 pixels/frame or 36 pixels/second @ 60 fps)

        //Player/environment collision detection v.2 (auto adjusts the player speed to prevent collision and detects when the player has hit the ground)
        isPlayerOnGround = false;
        handlePlayerEnvironmentCollision(inUseEnvironmentHitbox, inUseEnvironmentHitboxes, &playerPosition, &playerSpeed, &isPlayerOnGround, &canPlayerJump);
        
        //Update X offsets according to new player X speed 
        playerMiddleScreenOffsetX += playerSpeed.x;
        parallaxOffsetMountainX += playerSpeed.x;
        parallaxOffsetSkyX += playerSpeed.x;

        //PLAYER INTERFACE DATA

        //DRAW PLAYER, ENEMIES
        DrawRectangle(playerPosition.x, playerPosition.y, 30, 50, RED);         

        //CAMERA
        camera = (Camera2D){(Vector2){600, 500}, (Vector2){playerPosition.x, playerPosition.y}, 0, 0.3};   
        //Vector2 camerapos = GetWorldToScreen2D((Vector2){0, 0}, camera);  
        DrawRectangleLinesEx((Rectangle){-600 + playerPosition.x -playerSpeed.x,-500 + playerPosition.y -playerSpeed.y, 1280, 720}, 5, BLACK);   
        EndMode2D();
        //Beyond this point every drawing will be fixed and won't move with the camera

        //Player info overlay (health, xp, gold and so on)
    

        //Quick settings icon (last thing to be drawn because the pause menu needs to be generated when every action has already taken place)
        Color settingsIconColor = WHITE;
        if (checkMouseHoverRectangle((Rectangle){20, 20, 40, 40}))  //Change icon color if mouse is hovering
            settingsIconColor = YELLOW;        
        DrawTexture(settingsIcon, 20+3, 20+3, (Color){0, 0, 0, 127});       //Drop shadow
        DrawTexture(settingsIcon, 20, 20, settingsIconColor);
        //Quick settings screen (basically the pause menu)
        if ((checkMouseHoverRectangle((Rectangle){20, 20, 40, 40}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) { 
            EndDrawing();
            Image screenCapture = LoadImageFromScreen();    //Capture current screen
            Texture2D gameplayScreenCapture = LoadTextureFromImage(screenCapture);  //Set current screen as a background for the settings screen
            int exit = titleScreenSettings(true, overlayData, gameOptions, gameplayMusic2, gameplayScreenCapture, pixellariFont);
            if (exit == true || exit == 2) {    //If the user closed the window or wants to go back to title screen, unload used textures
                for (int i = 0; i < 5; i++)
                    UnloadTexture(mapScreen[i]);
                UnloadTexture(mountainBackground);
                UnloadTexture(skyBackground);
                UnloadTexture(settingsIcon);    
                UnloadSound(gameplayMusic);
                UnloadSound(gameplayMusic2);
            }
            if (exit == true) return true;      //User closed the window
            if (exit == 2) return false;        //User wants to go back to title screen
        }
        drawOverlayInfo(pixellariFont, overlayData);    //Draw any overlay info that was on queue
        EndDrawing(); 
    }
    for (int i = 0; i < 5; i++)
        UnloadTexture(mapScreen[i]);
    UnloadTexture(mountainBackground);
    UnloadTexture(skyBackground);
    UnloadTexture(settingsIcon);    
    UnloadSound(gameplayMusic);
    UnloadSound(gameplayMusic2);
    return true;   //Gameplay screen ended because the user closed the window
}

bool titleScreenControls(overlayVariables *overlayData, sharedOptions *gameOptions, Sound titleScreenMusic, Texture2D titleScreenBg, Font pixellariFont) {
    //Load Textures/Images
    //Idk yet
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
        DrawTextExCenterHover(pixellariFont, "RISK OF PAIN v.0.1 by Gabriel Pains", (Vector2){SCREEN_WIDTH/2, 0.98*SCREEN_HEIGHT}, 15, 0, WHITE, WHITE);    //Credits
        drawHighscore(gameOptions, pixellariFont);  //Current highscore
        
        //BACK
        if (DrawTextExCenterHover(pixellariFont, "BACK", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 230}, 35, 0, WHITE, YELLOW) == true || IsKeyPressed(KEY_ESCAPE))
            return false;       //Controls screen ended because the user clicked on "BACK" or pressed "ESC"
        drawOverlayInfo(pixellariFont, overlayData);        //Draw any overlay info that was on queue
        EndDrawing(); 
    }    
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
        DrawTextExCenterHover(pixellariFont, "RISK OF PAIN v.0.1 by Gabriel Pains", (Vector2){SCREEN_WIDTH/2, 0.98*SCREEN_HEIGHT}, 15, 0, WHITE, WHITE);    //Credits
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
    Font pixellariFont = LoadFont("fonts/Pixellari.ttf");

    //Load Bg Image
    Texture2D titleScreenBg = LoadTexture("images/background/title_screen_bg.png");

    //Loading Screen
    BeginDrawing();
    DrawTexture(titleScreenBg, 0, 0, WHITE);    //Background
    DrawTextExCenterHover(pixellariFont, "LOADING...", (Vector2){SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 30}, 35, 0, WHITE, WHITE);
    EndDrawing();

    //Load Sounds
    Sound titleScreenMusic = LoadSound("sounds/soundtrack/title_screen.wav");  
    
    //Shared screen variables
    sharedOptions gameOptions;
    overlayVariables overlayData;

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

    //De-Init  
    CloseAudioDevice();
    CloseWindow();    

    return 0;
}