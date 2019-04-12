#include <M5Stack.h>

enum Generation { Past = 0, Present = 1, Future = 2 };
enum Screen { Menu, Main };
enum CellSizes { One = 1, Two = 2, Four = 4, Eight = 8, Sixteen = 16 };

byte Cells[3][40][224];

boolean IsPaused = false;
int GenerationNumber = 0;

int NumberOfSurvivors = 0;
int NumberOfDeaths = 0;
int NumberOfBirths = 0;

Screen CurrentScreen;
CellSizes CellSize;

int DisplaySizeX = 320;
int DisplaySizeY = 220;

int GridSizeX;
int GridSizeY;

int ArraySizeX;
int ArraySizeY;

void setup()   {
  M5.begin();
  SetCellSize(Two);
  ChangeScreen(Menu);
}

void loop() {
  switch(CurrentScreen) {
    case Menu: MenuScreen(); break;
    case Main: MainScreen(); break;
  }
  M5.update();
}


void SetCellSize(CellSizes cellsize) {
  CellSize = cellsize;

  GridSizeX = DisplaySizeX / CellSize;
  GridSizeY = DisplaySizeY / CellSize;

  ArraySizeX = ceil(GridSizeX / 8.0);
  ArraySizeY = GridSizeY;
}

void ChangeScreen(Screen screen) {
  CurrentScreen = screen;
  switch(CurrentScreen) {
    case Menu:
      SetupMenuScreen();
      break;
    case Main:
      SetupMainScreen();
      break;
  }
}

void SetupMenuScreen() {
  ShowMenu();  
}

void ShowMenu() {
  M5.Lcd.fillScreen(TFT_BLACK);
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);

  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.printf("The Game of Life:\n\n");
  M5.Lcd.setTextColor(TFT_BLUE); 
  M5.Lcd.printf("Cell Size: %d\n", CellSize);
  M5.Lcd.printf("GridSizeX: %d\n", GridSizeX);
  M5.Lcd.printf("GridSizeY: %d\n", GridSizeY);
  
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.setCursor(0, 140);
  M5.Lcd.println("Press A - Dec Cell Size");
  M5.Lcd.println("Press B - Inc Cell Size");
  M5.Lcd.println("Press C - Start");
}

void MenuScreen() {
  if (M5.BtnA.wasPressed()) {
    DecrementCellSize();
    ShowMenu();
  }
  if (M5.BtnB.wasPressed()) {
    IncrementCellSize();
    ShowMenu();
  }
  if (M5.BtnC.wasPressed()) {
    ChangeScreen(Main);
  }
}

void DecrementCellSize() {
  switch(CellSize) {
    case One:     SetCellSize(Sixteen); break;
    case Two:     SetCellSize(One); break;
    case Four:    SetCellSize(Two); break;
    case Eight:   SetCellSize(Four); break;
    case Sixteen: SetCellSize(Eight); break;
  }
}

void IncrementCellSize() {
  switch(CellSize) {
    case One:     SetCellSize(Two); break;
    case Two:     SetCellSize(Four); break;
    case Four:    SetCellSize(Eight); break;
    case Eight:   SetCellSize(Sixteen); break;
    case Sixteen: SetCellSize(One); break;
  }
}

void SetupMainScreen() {
  CreateRandomCurrentGeneration();

  M5.Lcd.fillScreen(TFT_BLACK);
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setCursor(220, 230);
  M5.Lcd.printf("Step Pause Reset");
}

void MainScreen() {
  bool single_step = false;

  if (M5.BtnA.wasPressed()) {
    if (IsPaused) single_step = true;
  }

  if (M5.BtnB.wasPressed()) {
    IsPaused = !IsPaused;
  }

  if (M5.BtnC.wasPressed()) {
    ChangeScreen(Menu);
    return;
  }

  if (!IsPaused || single_step) {
    unsigned long start_millis = millis();

    int last_NumberOfDeaths = NumberOfDeaths;
    int last_NumberOfBirths = NumberOfBirths;

    NumberOfSurvivors = 0;
    NumberOfDeaths = 0;
    NumberOfBirths = 0;

    CalculateNextGeneration();
    DrawNextGeneration();
    PropogateGenerations();

    String status = "live";
    if ((last_NumberOfDeaths == NumberOfDeaths) && (last_NumberOfBirths == NumberOfBirths)) status = "stable";

    float fps = 1000.0 / (millis() - start_millis);

    M5.Lcd.fillRect(0, 230, 220, 10, TFT_BLACK);
    M5.Lcd.setCursor(0, 230);
    M5.Lcd.printf("G%04d F%3.1f S%04d D%03d B%03d %s", GenerationNumber, fps, NumberOfSurvivors, NumberOfDeaths, NumberOfBirths, status.c_str());

    GenerationNumber += 1;
  }
}

byte GetArrayValue(Generation generation, int x, int y) {
  return Cells[generation][x / 8][y];
}

boolean GetCellValue(Generation generation, int x, int y) {
  if ((x < 0) || (x >= GridSizeX) || (y < 0) || (y >= GridSizeY)) return false;
  
  byte array_value = GetArrayValue(generation, x, y);
  return (array_value >> (x % 8)) & 0x1;
}

void SetCellValue(Generation generation, int x, int y, boolean value) {
  if ((x < 0) || (x >= GridSizeX) || (y < 0) || (y >= GridSizeY)) return;

  byte array_value = GetArrayValue(generation, x, y);

  byte bit_mask = (0x1 << (x % 8));
  if (value)    array_value |=  bit_mask;
  else          array_value &= ~bit_mask;

  Cells[generation][x / 8][y] = array_value;
}

void DrawNextGeneration(void) {
  int color;
  
  for (int x = 0; x < GridSizeX; x++) {
    for (int y = 0; y < GridSizeY; y++) {
      if (GetCellValue(Present, x, y) != GetCellValue(Future, x, y)) {
        if (GetCellValue(Future, x, y)) {
          color = TFT_GREEN;
          NumberOfBirths += 1;
        }
        else {
          color = TFT_RED;
          NumberOfDeaths += 1;
        }
        M5.Lcd.fillRect(x * CellSize, y * CellSize, CellSize, CellSize, color);
      } else {
        if (GetCellValue(Present, x, y)) {
          NumberOfSurvivors += 1;
          color = TFT_WHITE;
        }
        else {
          color = TFT_BLACK;
        }

        if (GetCellValue(Present, x, y) != GetCellValue(Past, x, y))
          M5.Lcd.fillRect(x * CellSize, y * CellSize, CellSize, CellSize, color);
      }
    }
  }
}

void PropogateGenerations() {
  for (int x = 0; x < ArraySizeX; x++) {
    for (int y = 0; y < ArraySizeY; y++) {
      Cells[Past][x][y] = Cells[Present][x][y];
      Cells[Present][x][y] = Cells[Future][x][y];
    }
  }
}

void CreateRandomCurrentGeneration(void) {
  GenerationNumber = 0;
  for (int x = 0; x < GridSizeX; x++) {
    for (int y = 0; y < GridSizeY; y++) {
      SetCellValue(Present, x, y, random(2));
    }
  }
}

void CalculateNextGeneration() {
  for (int x = 0; x < GridSizeX; x++) {
    for (int y = 0; y < GridSizeY; y++) {
      byte neighbors = NumberOfNeighbors(x, y);
      
      if (GetCellValue(Present, x, y))  SetCellValue(Future, x, y, (neighbors == 2 || neighbors == 3 ));
      else                              SetCellValue(Future, x, y, (neighbors == 3));
    }
  }
}

byte NumberOfNeighbors(int x, int y) {
  byte neighbors = 0;
  
  if (GetCellValue(Present, x - 1, y)) neighbors += 1;
  if (GetCellValue(Present, x - 1, y - 1)) neighbors += 1;
  if (GetCellValue(Present, x, y - 1)) neighbors += 1;
  if (GetCellValue(Present, x + 1, y - 1)) neighbors += 1;
  if (GetCellValue(Present, x + 1, y)) neighbors += 1;
  if (GetCellValue(Present, x + 1, y + 1)) neighbors += 1;
  if (GetCellValue(Present, x, y + 1)) neighbors += 1;
  if (GetCellValue(Present, x - 1, y + 1)) neighbors += 1;

  return neighbors;
}