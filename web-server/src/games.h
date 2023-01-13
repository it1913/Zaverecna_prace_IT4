// #include <game.h>

// class StopWatch : public Game
// /*  ===== Stopky =====
//     Parametr simple:
//     Hodnota "false" urcuje, ze se pouziva pouze jedno
//     tlacitko - cilove. Startuje starter z aplikace, hrac reaguje
//     na rozsviceni ciloveho tlacitka a v tu chvili zacina
//     bezet cas. Cas je zastaven pri stisku koncoveho tlacitka.

//     Hodnota "true" urcuje, ze jsou pouzita dve tlacitka - startovaci
//     a cilove. Starter z aplikace rozsviti startovaci tlacitko
//     a cas se zacina merit v okamziku, kdy hrac stiskne startovaci
//     tlacitko. Konec mereni je opet pri stisku koncoveho tlacitka.
// */ 
// {
//     private:
//         bool _simple;
//     public:
//         StopWatch(bool simple) 
//         : Game(2) {
//             _simple = simple;
//             _sameButtonIsAllowed = false;
//         }
// };

// class ShuttleRun : public Game
// /*  ===== Clunkovy beh =======
//     Pouzita jsou dve tlacitka, hrac mezi nimi beha predem urceny pocet krat.
//     Starter spusti hru, rozsviti se startovaci tlacitko, cas zacne bezet
//     v okamziku, kdy ho stiskne hrac.

//     Cviceni 
// */
// {
//     public:
//         ShuttleRun(int AStepCount)
//         : Game(AStepCount) {

//         }
// };

// class Fan : public Game 
// {
//     private:
//         int _pivot;
//     public:
//         Fan(int pivot, int stepCount)
//         : Game(stepCount) {
//             _pivot = pivot;
//             _sameButtonIsAllowed = false;
//         }
// };