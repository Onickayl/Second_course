#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

union semun
{
    int val;               // значение
    struct semid_ds *buf;  // буффер для IPC_STAT, IPC_SET
    unsigned short *array; // массив для GETALL, SETALL
} arg;

int sem_wait(int semid, int sem_num);
int sem_signal(int semid, int sem_num);

int passendger(int semid, int i);
void capitan(int semid, int people);

void passendger_want_trip(int semid, int i);
void passendger_want_earth(int semid, int i);

int main(int argc, char *argv[])
{
    if (argc != 5)
    { // no argv
        printf("\nPLEASE WRITE: ./name N M num_all_people num_all_trips\n\n");
        return 0;
    }

    int boat = atoi(argv[1]);
    int trap = atoi(argv[2]);
    int people = atoi(argv[3]);
    int trips = atoi(argv[4]);


    key_t key = IPC_PRIVATE;

    // создаём 
    int semid = semget(key, 9, IPC_CREAT | IPC_EXCL | 0666);
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
    6 captain_signal  // сигнал капитану
    7 e_perm          // разрешение войти с земли
    8 b_perm          // разрешение выйти с борта
    */

    union semun arg;
    unsigned short values[9] = {trap, boat, 0, 0, 0, trips, 0, 0, 0}; // начальные значения
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1)
    {
        perror("semctl SETALL");
        exit(1);
    }

    // Создаём процессы-люди
    for (int i = 0; i < people; i++)
    {
        if (fork() == 0)
        {
            passendger(semid, i+1);
            exit(0);
        }
    }

    // Капитан (родительский процесс)
    capitan(semid, people);

    // Ждём завершения всех людей
    for (int i = 0; i < people; i++)
    {
        wait(NULL);
    }

    // удаление
    semctl(semid, IPC_RMID, 0);

    return 0;
}


int passendger(int semid, int i)
{
    while (1)
    {
        int trips = semctl(semid, 5, GETVAL);
        if (trips <= 0)
        {
            break; // Выходим, если поездок нет
        }

        passendger_want_trip(semid, i);

        // Проверяем ещё раз (могли закончиться во время ожидания)
        trips = semctl(semid, 5, GETVAL);
        if (trips <= 0) 
        {
            break;
        }

        printf("чел %d любуется пейзажем, пока ждёт отплытие\n", i);
        sleep(1);

        passendger_want_earth(semid, i);
        printf("чел %d на земле\n", i);
        sleep(1);
    }

    return 0;
}

void capitan(int semid, int people)
{

    /* Индексы семафоров в массиве
    0 free_trap       // счетчик свободных мест на трапе
    1 free_boat       // счётчик свободных мест на борту
    2 free_earth      // счётчик свободных мест на земле
    3 etr_wait        // количество ожидающих на трап с земли
    4 btr_wait        // количество ожидающих на трап с корабля
    5 trips           // счётчик трипов
    6 captain_signal  // сигнал капитану
    7 permission      // разрешение от капитан
    */
    while (1)
    {
        int trips = semctl(semid, 5, GETVAL); // трипов
        if (trips <= 0)
        {
            break;
        }

        
        int btr_wait = semctl(semid, 4, GETVAL);

        // ждёт желающих сойти
         while (1)
        {
            int btr_wait = semctl(semid, 4, GETVAL);
            if (btr_wait <= 0) 
            {
                break; // Нет желающих выйти
            }
            
            // Ждём сигнала от пассажира
            sem_wait(semid, 6);
            
            // Разрешаем выйти
            sem_signal(semid, 8);
            
            // Ждём подтверждения выхода
            sem_wait(semid, 6);
            
        }

        
        int free_boat = semctl(semid, 1, GETVAL);  // свободных мест на борту
        int etr_wait = semctl(semid, 3, GETVAL);

        // ждёт желающих войти
        if (etr_wait > 0 && free_boat > 0) 
        {
            // Обрабатываем пока есть желающие и места
            while (1) 
            {
                int current_etr_wait = semctl(semid, 3, GETVAL);
                int current_free_boat = semctl(semid, 1, GETVAL);
                
                if (current_etr_wait <= 0 || current_free_boat <= 0) break;
                
                sem_signal(semid, 7); // разрешаем войти
                sem_wait(semid, 6);   // ждём подтверждения
            }
        }

        free_boat = semctl(semid, 1, GETVAL);
        int free_earth = semctl(semid, 2, GETVAL); // свободных мест на земле

        // можно ли отплывать?
        if (free_boat == 0 || free_earth == people)
        {
            printf("катер отплывает\n");
            printf("катер плавает\n");
            sleep(1); // имитация плаванья
            printf("катер возвращается\n");

            sem_wait(semid, 5); // trips--
        }
    }

    printf("Все поездки завершены!\n");
}

void passendger_want_trip(int semid, int i)
{ // хочет на борт

    printf("человек %d хочет зайти на борт\n", i);

    sem_signal(semid, 3); // etrap_wait++ ждущих вход на трап с земли

    // а катер вообще есть?
    sem_signal(semid, 6); // сиг кэпу ты тут?
    sem_wait(semid, 7);   // ждём разрешение
    

    // заходим на трап
    sem_wait(semid, 0); // free_trap--
    printf("человек %d зашёл на трап с земли\n", i);

    // заходим на борт
    sem_wait(semid, 1); // free_boat--
    printf("человек %d зашёл на борт\n", i);
    sem_signal(semid, 0); // free_trap++
    sem_signal(semid, 2); // free_earth++
    sem_wait(semid, 3);   // eatrap_wait--

    sem_signal(semid, 6); // сиг кэпу, что зашли
}

/* Индексы семафоров в массиве
0 free_trap       // счетчик свободных мест на трапе
1 free_boat       // счётчик свободных мест на борту
2 free_earth      // счётчик свободных мест на земле
3 etr_wait        // количество ожидающих на трап с земли
4 btr_wait        // количество ожидающих на трап с корабля
5 trips           // счётчик трипов
6 captain_signal  // сигнал капитану
7 permission      // разрешение от капитан
*/

void passendger_want_earth(int semid, int i)
{ // хочет на землю

    printf("человек %d хочет сойти на землю\n", i);

    sem_signal(semid, 4); // btrap_wait++ ждущих вход на трап

    sem_signal(semid, 6); // сиг кэпу, что хотим выйти
    sem_wait(semid, 8);   // разрешение на выйти

    // заходим на трап
    sem_wait(semid, 0); // free_trap--
    printf("человек %d зашёл на трап с борта\n", i);
    sem_signal(semid, 1); // free_boat++

    // сходим на землю
    sem_wait(semid, 2); // free_earth--
    printf("человек %d сошёл на землю\n", i);
    sem_signal(semid, 0); // free_trap++
    sem_wait(semid, 4);   // btr_wait--

    sem_signal(semid, 6); // сиг кэпу, что сошли

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