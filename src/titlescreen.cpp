
#include "titlescreen.h"
#include <string>
#include "SaveSystem.h"

// Ellipsize helper
static std::string Ellipsize(const std::string& s, size_t maxLen = 38) {
    if (s.size() <= maxLen) return s;
    if (maxLen <= 3) return s.substr(0, maxLen);
    return s.substr(0, maxLen - 3) + "...";
}

void TitleScreen::Render(SDL_Surface *src, SDL_Surface *dest, Font &font)
{
    SDL_BlitSurface(src, nullptr, dest, nullptr);
    bool flash = (timer / 5) & 1;

    font.PrintMessage("ZGLOOM PSVITA 11.2025", 245, dest, 1);

    if (status == TITLESTATUS_MAIN)
    {
        const bool hasResume = SaveSystem::HasSaveForCurrentGame();
        const int idxStart   = hasResume ? 1 : 0;
        const int idxSelect  = idxStart + 1;
        const int idxGame    = idxStart + 2;
        const int idxQuit    = idxStart + 3;

        int y = 150;

        if (hasResume) {
            if (flash || (selection != 0)) font.PrintMessage("RESUME SAVED POSITION", y, dest, 1);
            y += 15;
        }

        if (flash || (selection != idxStart))  font.PrintMessage("START NEW GAME", y, dest, 1);
        y += 15;
        if (flash || (selection != idxSelect)) font.PrintMessage("SELECT LEVEL", y, dest, 1);
        y += 15;
        if (flash || (selection != idxGame))   font.PrintMessage("ABOUT GLOOM AND THIS PORT", y, dest, 1);
        y += 15;
        if (flash || (selection != idxQuit))   font.PrintMessage("EXIT", y, dest, 1);
    }
    else if (status == TITLESTATUS_SELECT)
    {
        for (int i = selection - 10; i < selection + 10; i++)
        {
            if ((i >= 0) && (i < (int)levelnames.size()))
            {
                const std::string label = Ellipsize(levelnames[i], 38);
                if (flash || (i != selection)) {
                    font.PrintMessage(label, 100 + (i - selection) * 10, dest, 1);
                } else {
                    font.PrintMessage(label, 100 + (i - selection) * 10, dest, 2);
                }
            }
        }
    }
    else if (status == TITLESTATUS_GAME)
    {
		font.PrintMessage("GLOOM BLACK MAGIC ENGINE", 40, dest, 1);
		font.PrintMessage("BY BLACK MAGIC", 50, dest, 1);

		font.PrintMessage("PROGRAMMED BY MARK SIBLY", 65, dest, 1);
		font.PrintMessage("GRAPHICS BY THE BUTLER BROTHERS", 75, dest, 1);
		font.PrintMessage("MUSIC BY KEV STANNARD", 85, dest, 1);
		font.PrintMessage("AUDIO BY BLACK MAGIC", 95, dest, 1);
		font.PrintMessage("DECRUNCHCODE BY THOMAS SCHWARZ", 105, dest, 1);

		font.PrintMessage("GLOOM3 AND ZOMBIE MASSACRE", 120, dest, 1);
		font.PrintMessage("BY ALPHA SOFTWARE", 130, dest, 1);

		font.PrintMessage("ABOUT THIS PORT", 145, dest, 1);
		font.PrintMessage("CODE AND FIXES BY ANDIWELI", 160, dest, 1);
		font.PrintMessage("AMBIENCE BY PROPHET", 170, dest, 1);
		font.PrintMessage("BASED ON PORT BY SWIZPIG", 185, dest, 1);

		font.PrintMessage("IN HONOR AND MEMORY OF MARK SIBLY", 200, dest, 1);
    }
}

TitleScreen::TitleScreen()
{
    status = TITLESTATUS_MAIN;
    selection = 0;
    timer = 0;
}

TitleScreen::TitleReturn TitleScreen::Update(int &levelout)
{
    if (status == TITLESTATUS_MAIN)
    {
        const bool hasResume = SaveSystem::HasSaveForCurrentGame();
        const int count = hasResume ? 5 : 4;
        const int idxStart   = hasResume ? 1 : 0;
        const int idxSelect  = idxStart + 1;
        const int idxGame    = idxStart + 2;
        const int idxQuit    = idxStart + 3;

        if (Input::GetButtonDown(SCE_CTRL_DOWN))
        {
            selection++;
            if (selection >= count) selection = 0;
        }

        if (Input::GetButtonDown(SCE_CTRL_UP))
        {
            selection--;
            if (selection < 0) selection = count - 1;
        }

        if (Input::GetButtonDown(SCE_CTRL_CROSS))
        {
            if (hasResume && selection == 0) { return TITLERET_RESUME; } 
            if (selection == idxStart)  return TITLERET_PLAY;
            if (selection == idxQuit)   return TITLERET_QUIT;
            if (selection == idxGame)   status = TITLESTATUS_GAME;
            if (selection == idxSelect)
            {
                selection = 0;
                status = TITLESTATUS_SELECT;
            }
        }
    }
    else if (status == TITLESTATUS_SELECT)
    {
        // Back from Level Select
        if (Input::GetButtonDown(SCE_CTRL_CIRCLE))
        {
            status = TITLESTATUS_MAIN;
            selection = 0;
        }

        if (Input::GetButtonDown(SCE_CTRL_DOWN))
        {
            selection++;
            if (selection == (int)levelnames.size())
                selection = 0;
        }

        if (Input::GetButtonDown(SCE_CTRL_UP))
        {
            selection--;
            if (selection == -1)
                selection = (int)levelnames.size() - 1;
        }

        if (Input::GetButtonDown(SCE_CTRL_CROSS))
        {
            levelout = selection;
            status = TITLESTATUS_MAIN;
            selection = 0;
            return TITLERET_SELECT;
        }
    }
    else
    {
        // About screens: back to MAIN
        if (Input::GetButtonDown(SCE_CTRL_CIRCLE))
            status = TITLESTATUS_MAIN;
        else if (Input::GetButtonDown(SCE_CTRL_CROSS))
            status = TITLESTATUS_MAIN;
    }

    return TITLERET_NOTHING;
}
