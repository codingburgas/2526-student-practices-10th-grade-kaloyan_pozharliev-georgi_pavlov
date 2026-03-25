#include "app.h"

int main()
{
    App* app = new App({ 1440, 800 }, "Gekoya");
    app->Run();
    delete app;
}