// new module header
import void SaveView(int view);
import int GetAddressOfView(int view);
import int RestoreFrameGraphic(int view, int loop, int frame, int address);
import function UpdateView(this Character*, int view);
import function CreateLightX(int id, int x, int y, int radius, bool state);
import bool CharacterCloseToLight(this Character*,int id, int byradius);
import int DistToL (this Character*, int id);