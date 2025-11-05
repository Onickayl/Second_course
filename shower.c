#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/sem.h>

union semun
{
    int val;               // значение
    struct semid_ds *buf;  // буффер для IPC_STAT, IPC_SET
    unsigned short *array; // массив для GETALL, SETALL
} arg;

int creat_man(int semid, int num_M, int num_place);
int creat_woman(int semid, int num_W, int num_place);

int sem_wait(int semid, int sem_num);
int sem_signal(int semid, int sem_num);

int can_enter(int semid, int free, int gender, int prioritet, int gen);
int enter(int semid, int i, int gender, int gen);
int exit_shower(int semid, int num_place, int free, int m_wait, int w_wait, int gender, int prioritet, int gen, int i);

int man_process(int semid, int i, int num_place);
int woman_process(int semid, int i, int num_place);


/*

Процессы:

    Каждый процесс имеет гендер (M/W)

    Процессы пытаются войти в душевую

Условия входа:

    Если душевая пустая: входит только процесс с приоритетным гендером

    Если в душевой есть люди: входят только процессы того же гендера

    Если войти нельзя - процесс становится в очередь

Приоритет:

    Когда душевая пустая, первыми заходят процессы с приоритетным гендером

Синхронизация:

    Используются очереди для каждого гендера

    При выходе из душа пробуждаются ожидающие процессы


// Индексы семафоров в массиве
    0 mutex           // взаимное исключение для критических секций
    1 free    // счетчик свободных мест в душевой
    2 m_wait      // количество ожидающих мужчин
    3 w_wait      // количество ожидающих женщин
    4 prioritet     // приоритет (0 - мужчина, 1 - женщина)
    5 gender  // текущий гендер в душевой (0 - пусто, 1 - мужчины, 2 - женщины)

*/

int main(int argc, char *argv[])
{
// Начальные значения
    if (argc != 5)
    {
        printf("write './name' 'num_place' 'num_of_M' 'num_of_W' 'prioritet(M/W)'\n'");
        return 0;
    }

    int num_place = atoi(argv[1]);
    int num_M = atoi(argv[2]);
    int num_W = atoi(argv[3]);
    char *prioritet = argv[4];

// Создание набора семафоров
    int semid = semget(1234, 6, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(1);
    }

    // Инициализация семафоров
    /* Индексы семафоров в массиве
    0 mutex           // взаимное исключение для критических секций
    1 free            // счетчик свободных мест в душевой
    2 m_wait          // количество ожидающих мужчин
    3 w_wait          // количество ожидающих женщин
    4 prioritet       // приоритет (0 - мужчина, 1 - женщина)
    5 gender          // текущий гендер в душевой (0 - пусто, 1 - мужчины, 2 - женщины)
    */

    union semun arg;
    unsigned short values[6] = {1, num_place, 0, 0, 0, 0}; // начальные значения
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1)
    {
        perror("semctl SETALL");
        exit(1);
    }

    if (strcmp(prioritet, "M") == 0)
    {
        creat_man(semid, num_M, num_place);
        sleep(1);
        creat_woman(semid, num_W, num_place);
    }
    else
    {
        sem_signal(semid, 4);
        creat_woman(semid, num_W, num_place);
        sleep(1);
        creat_man(semid, num_M, num_place);
    }

// Ожидание
    for (int i = 0; i < num_M + num_W; i++)
    {
        wait(NULL);
    }

// удаление
    semctl(semid, IPC_RMID, 0);

    return 0;
}


int man_process(int semid, int i, int num_place)
{
    int gen = 1;

    printf("Мужчина %d хочет зайти в душ\n", i);

    sem_signal(semid, 2); // m_wait ++

    int entered = 1;

    while (entered)
    {
// вход в кр секцию
        sem_wait(semid, 0); // mutex == 0

        int free = semctl(semid, 1, GETVAL);
        int m_wait = semctl(semid, 2, GETVAL);
        int w_wait = semctl(semid, 3, GETVAL);
        int prioritet = semctl(semid, 4, GETVAL); // 0 - м, 1 - w
        int gender = semctl(semid, 5, GETVAL);      // 0 - пусто, 1 - м, 2 - ж

        if (can_enter(semid, free, gender, prioritet, gen))
        {
            enter(semid, i, gender, gen);

            // Процесс принятия душа
            printf("Мужчина %d принимает душ\n", i);
            sleep(5); // имитация принятия душа
            printf("Мужчина %d закончил мыться\n", i);

            exit_shower(semid, num_place, free, m_wait, w_wait, gender, prioritet, gen, i);

            entered = 0;
        }
        else
        {
            // Не можем войти сейчас - ждем
            printf("Мужчина %d ждет\n", i);
            sem_signal(semid, 0); // mutex == 1
            sleep(5);             // Ждем перед следующей попыткой
        }
    }
    return 0;
}

int woman_process(int semid, int i, int num_place)
{
    int gen = 2; 

    printf("женщина %d хочет зайти душ\n", i);

    // количество ожидающих женщин
    sem_signal(semid, 3); // w_wait ++

    int entered = 1;

    while (entered)
    {
// вход в крит секцию
        sem_wait(semid, 0); // mutex == 0

        int free = semctl(semid, 1, GETVAL);
        int m_wait = semctl(semid, 2, GETVAL);
        int w_wait = semctl(semid, 3, GETVAL);
        int prioritet = semctl(semid, 4, GETVAL); // 0 - м, 1 - w
        int gender = semctl(semid, 5, GETVAL);      // 0 - пусто, 1 - м, 2 - ж

        if (can_enter(semid, free, gender, prioritet, gen))
        {
            enter(semid, i, gender, gen);

            // Процесс принятия душа
            printf("женщина %d принимает душ\n", i);
            sleep(5); // имитация принятия душа
            printf("женщина %d закончила мыться\n", i);

            exit_shower(semid, num_place, free, m_wait, w_wait, gender, prioritet, gen, i);

            entered = 0;
        }
        else
        {
            // Не можем войти сейчас - ждем
            printf("женщина %d ждет\n", i);
            sem_signal(semid, 0); // mutex == 1
            sleep(5);             // Ждем перед следующей попыткой
        }
    }

    return 0;
}

// Функция проверки возможности входа
int can_enter(int semid, int free, int gender, int prioritet, int gen)
{
    /*Условия входа:
        1. Есть свободное место
        2. Душевая пуста
        3. в душевой тот же ген
        4. Приоритетный
    */

    if (free > 0 && (gender == 0 || gender == gen) && prioritet == gen-1) //here
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// функция входа
int enter(int semid, int i, int gender, int gen)
{
    if (gen == 1)
    {
        printf("Мужчина %d зашел в душ\n", i);
    }
    if (gen == 2)
    {
        printf("женщина %d зашла в душ\n", i);
    }

    sem_wait(semid, 1); // free--

    // Устанавливаем гендер, если душевая была пуста
    if (gender == 0)
    {
        semctl(semid, 5, SETVAL, gen);
    }

    sem_wait(semid, gen + 1); // _wait--

    // выход из крит секции
    sem_signal(semid, 0); // mutex == 1
}

// функция выхода из душа
int exit_shower(int semid, int num_place, int free, int m_wait, int w_wait, int gender, int prioritet, int gen, int i)
{
// вход в крит секцию
    sem_wait(semid, 0); // mutex == 0

    if (gen == 1)
    {
        printf("Мужчина %d вышел из душа\n", i);
    }
    if (gen == 2)
    {
        printf("женщина %d вышла из душа\n", i);
    }

    sem_signal(semid, 1); // free++

    // Проверяем, нужно ли сменить гендер
    free = semctl(semid, 1, GETVAL);
    m_wait = semctl(semid, 2, GETVAL);
    w_wait = semctl(semid, 3, GETVAL);
    gender = semctl(semid, 5, GETVAL);

    // Если душевая полностью пуста
    if (free == num_place)
    {
        // Если есть ожидающие женщины и нет ожидающих мужчин
        if (w_wait > 0 && m_wait == 0 )
        {
            // Смена гендера на женщин
            semctl(semid, 5, SETVAL, 2);
            printf("        Смена гендера: теперь женщины\n");
            semctl(semid, 4, SETVAL, 1);
        }
        // Если есть ожидающие мужчины и нет ожидающих женщин
        if (m_wait > 0 && w_wait == 0)
        {
            // Смена гендера на женщин
            semctl(semid, 5, SETVAL, 1);
            printf("        Смена гендера: теперь мужчины\n");
            semctl(semid, 4, SETVAL, 0);
        }
        // Если есть ожидающие мужчины
        else if (m_wait > 0 && w_wait > 0)
        {
            if(prioritet == 0)
            {
                // Остаемся мужчинами или устанавливаем мужчин
                semctl(semid, 5, SETVAL, 1);
                printf("        Душевая: остаются мужчины\n");
            }
            else
            {
                // Остаемся женщинами или устанавливаем женщин
                semctl(semid, 5, SETVAL, 2);
                printf("        Душевая: остаются женщины\n");
            }            
        }
        else
        {
            // Никого нет - душевая пуста
            semctl(semid, 5, SETVAL, 0);
            printf("        Душевая: пуста\n");
        }

    }

// выход из крит секции
    sem_signal(semid, 0); // mutex == 1
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

int creat_man(int semid, int num_M, int num_place)
{
    for (int i = 1; i <= num_M; i++)
    {
        if (fork() == 0)
        {
            man_process(semid, i, num_place);
            exit(0);
        }
    }

    return 0;
}

int creat_woman(int semid, int num_W, int num_place)
{
    for (int i = 1; i <= num_W; i++)
    {
        if (fork() == 0)
        {
            woman_process(semid, i, num_place);
            exit(0);
        }
    }

    return 0;
}