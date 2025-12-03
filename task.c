#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>


union semun
{
    int val;               // значение
    struct semid_ds *buf;  // буффер для IPC_STAT, IPC_SET
    unsigned short *array; // массив для GETALL, SETALL
} arg;


int sem_wait(int semid, int sem_num);
int sem_signal(int semid, int sem_num);

int passendger(int semid, int people);
void capitan(int semid, int people);

void passendger_want_trip(int semid);
void passendger_want_earth(int semid);



int main(int argc, char *argv[])
{
    if (argc != 5)
    { // no argv
        printf("write: ./name N M num_all_people num_all_trips\n");
        return 0;
    }

    int boat = atoi(argv[1]);
    int trap = atoi(argv[2]);
    int people = atoi(argv[3]);
    int trips = atoi(argv[4]);


    // Создание набора семафоров
    int semid = semget(1234, 6, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(1);
    }


    // Инициализация семафоров
    /* Индексы семафоров в массиве
    0 free_trap       // счетчик свободных мест на трапе
    1 free_boat       // счётчик свободных мест на борту
    2 free_earth      // счётчик свободных мест на земле
    3 etr_wait        // количество ожидающих на трап с земли
    4 btr_wait        // количество ожидающих на трап с корабля
    5 trips           // счётчик трипов
    6 signal          // взаимодействие капитана и людей
    */

    union semun arg;
    unsigned short values[6] = {trap, boat, 0, 0, 0, trips, 0}; // начальные значения
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1)
    {
        perror("semctl SETALL");
        exit(1);
    }

//прога
    pid_t p = fork();

    if(p == 0)
    {
        passendger(semid, people);

    }
    else
    {
        capitan(semid, people);
        wait(NULL);
    }

// удаление
    semctl(semid, IPC_RMID, 0);

    return 0;
}




int passendger(int semid, int people)
{
    while(is_boat())
    {
        passendger_want_trip(semid);
        if(is_boat())
        {
            printf("путешествует\n");
            sleep(5);
            passendger_want_earth(semid);
            printf("на берегу\n");
        }
        else
        {
            return 0;
        }
    
    }

    return 0;
    
}

is_boat() // что есть катер и можно покататься
{

}

void capitan(int semid, int people)
{

    int free_trap = semctl(semid, 0, GETVAL); // свободных мест на трапе
    int free_boat = semctl(semid, 1, GETVAL); // свободных мест на борту
    int free_earth = semctl(semid, 2, GETVAL); // свободных мест на земле
    int etr_wait = semctl(semid, 3, GETVAL);
    int btr_wait = semctl(semid, 4, GETVAL);
    int trips = semctl(semid, 5, GETVAL); // трипов


    // пока мы не совершили все рейсы - катаем людей
    while(trips > 0)
    {
        
        //ждёт желающих на борт и желающих сойти
        if((etr_wait > 0 && free_boat > 0) || btr_wait > 0)
        {
            //cap wait 
        }

        if(free_boat == 0 || free_earth == people)
        {
            printf("катер отплывает\n");
            printf("катер плавает\n");
            sleep(5);   // имитация плаванья
            printf("катер возвращается\n");

            sem_wait(semid, 5); //trips--

            //кэп посылает сигнал, что может покатать?
        }
    }
}


void passendger_want_trip(int semid)
{//хочет на борт

    print("человек хочет зайти на борт\n");

    sem_signal(semid, 3);   //etrap_wait++ ждущих вход на трап с земли


    int free_trap = semctl(semid, 0, GETVAL); // свободных мест на трапе
    int free_boat = semctl(semid, 1, GETVAL); // свободных мест на борту
    int free_earth = semctl(semid, 2, GETVAL); // свободных мест на земле
    int etr_wait = semctl(semid, 3, GETVAL);

    // проверить место на трапе и на борту
    if(free_trap > 0 && free_boat > 0) // есть место на трапе и борту
    {

        // заходим на трап
        printf("человек зашёл на трап с земли\n");
        sem_signal(semid, 2);//free_earth++
        sem_wait(semid, 0);//free_trap--
        sem_wait(semid, 3);//eatrap_wait--

        //заходим на борт
        printf("человек заходит на борт");
        sem_signal(semid, 2);//free_earth++
        sem_wait(semid, 1);//free_boat--


    }
    else if(free_trap = 0 && free_boat > 0) // есть место на борте, но нет на трапе
    {
        // ждём когда можно зайти на трап
        printf("человек ждет места на трапе - земля\n");

    }
    else // нет место на борту
    {
        //ждём следующего трипа
        printf("человек ждет следующего трипа\n");
    }


}

void passendger_want_earth(int semid)
{//хочет на землю

    print("человек хочет сойти на землю\n");

    sem_signal(semid, 3);   //btrap_wait++ ждущих вход на трап


    int free_trap = semctl(semid, 0, GETVAL); // свободных мест на трапе
    int free_boat = semctl(semid, 1, GETVAL); // свободных мест на борту
    int free_earth = semctl(semid, 2, GETVAL); // свободных мест на земле
    int btr_wait = semctl(semid, 4, GETVAL);

    // проверить место на трапе
    if(free_trap > 0)
    {   

        // заходим на трап
        printf("человек зашёл на трап с борта\n");
        sem_wait(semid, 4);//btrap_wait--
        sem_signal(semid,12);//free_boat++
        sem_wait(semid, 0);//free_trap--


        // сходим на землю
        printf("человек сошёл на землю\n");
        sem_wait(semid, 0);//free_trap--
        sem_signal(semid, 2);//free_earth++


    }
    else
    {
        // ждём когда можно зайти на трап

        printf("человек ждет места на трапе - борт\n");
    }
    
    
}



// Операция P (wait)
int sem_wait(int semid, int sem_num)
{
    struct sembuf op = {sem_num, -1, 0};
    return semop(semid, &op, 1);
}

// Операция V (signal)
int sem_signal(int semid, int sem_num)
{
    struct sembuf op = {sem_num, 1, 0};
    return semop(semid, &op, 1);
}