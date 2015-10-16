/*
 * OS Assignment #2
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

#define ID_SIZE         3
#define COMMAND_SIZE    128
#define TASK_SIZE       260
#define TIME            30

typedef enum state {
    NONE,
    READY,
    RUNNING,
    WAITING,
    EXIT

} state;

typedef struct task {
    int index;
    char id[ID_SIZE];
    int arrive_time;
    int service_time;
    int priority;

    state task_state;
    int remain_time;

} task;


typedef struct node {
    int value;
    struct node *next;

} node;

node *head;

task tasks[TASK_SIZE];
task origin_tasks[TASK_SIZE];
int num_of_task;

/* copy original value */
void copy_origin() {
    int n;
    for(n=0; n < TASK_SIZE; ++n) {
        tasks[n] = origin_tasks[n];
    }
}

/* init node */
void initList() {
    head = (node*)malloc(sizeof(node));
    head->next = NULL;
}


/* add value in rear of list */
void addNode(int value) {
    node *new;
    new = (node*)malloc(sizeof(node));

    new->value = value;
    new->next = NULL;
    
    node* temp = head;
    while(temp->next != NULL) {
        temp = temp->next; 
    }
    
    temp->next = new;
}


/* delete node in list */
void removeNode() {
    node* temp;
    temp = head->next;

    head->next = NULL;
    free(head);

    head = temp; 
}

/* add value into first list */
void addNodeFirst(int value) {
    node *new;
    new = (node*)malloc(sizeof(node));

    new->value = value;
    new->next = head;
    head = new;
}


/* id 중복 체크 */
int check_duplicated_id(const char *id) {
    int i;
    for(i=0; i < num_of_task; ++i) {
        if(strcmp(id, tasks[i].id) == 0) {
            return -1;
        }
    }

    return 0;
}


/* id 체크 */
int check_valid_id(const char *id) {
    int id_len = strlen(id);

    if(id_len != 2) { // id 길이가 2가 아닐 경우
        return -1;
    }

    if(id[0] < 65 && id[0] > 90) { // 첫 번째 문자열 대문자 확인
        return -1;
    }

    if(!isdigit(id[1])) { // 두 번째 문자열 숫자 확인
        return -1;
    }

    return 0;
}


/* 도착 시간 유효성 체크 */
int check_arrive_time(const int arrive_time) { 
    if(arrive_time < 0 || arrive_time > TIME) {
        return -1;
    }
    
    return 0;
}


/* 서비스 시간 유효성 체크 */
int check_service_time(const int service_time) { 
    if(service_time < 1 || service_time > TIME) {
        return -1;
    }

    return 0;
}


/* 우선순위 유효성 체크 */
int check_priority(const int priority) { 
    if(priority < 1 || priority > 10) {
        return -1;
    }

    return 0;
}


/* 결과값 그래프 그리기 */
void draw_graph(int cpu_time, int result[TASK_SIZE]) {
    int i, j;

    for(i=0; i < num_of_task; ++i) {
        printf("%s ", tasks[i].id);
        for(j=0; j < cpu_time+1; ++j) {
            if(result[j] == i) {
                printf("*");

            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
}


/* get waiting time */
double get_waitingTime(int cpu_time, int result[TASK_SIZE]) {
    int i, j;
    double res = 0;
    int pivot[num_of_task];
    int start = 0;

    memset(pivot, 0, sizeof(pivot));

    for(i=0; i < num_of_task; ++i) {
        start = 0;
        pivot[i] = tasks[i].arrive_time;

        for(j=0; j < cpu_time; ++j) {
            if(i == result[j]) {
                if(start == 0) {
                    start = 1;
                    int temp = (j-pivot[i]);
                    res += temp;

                } else {
                    continue;
                }
            } else {
                if(start == 1) {
                    pivot[i] = j;
                    start = 0;
                }
            }
        }
    }

    res /= num_of_task;

    return res;
}


/* get turnaround time */
double get_turnaroundTime(int cpu_time, int result[TASK_SIZE]) {
    int i, j;
    double res = 0;
    int arrive_time[num_of_task];
    int start = 0;

    memset(arrive_time, 0, sizeof(arrive_time));
    
    for(i=0; i < num_of_task; ++i) {
        start = 0;
        arrive_time[i] = tasks[i].arrive_time;

        for(j=0; j < cpu_time; ++j) {
            if(i == result[j]) {
                if(start == 0) {
                    start = 1;
                }
            } else {
                if(start == 1) {
                    res += (j-arrive_time[i-1]);
                    start = 0;
                    arrive_time[i-1] = j;
                }
            }
        }
    }

    res /= num_of_task;

    return res;
}


/* shortest-job-first scheduling */
void SJF(task process[TASK_SIZE]) { 
    int cpu_time = 0;
    float turnaround_time = 0;
    float waiting_time = 0;
    
    int result[TASK_SIZE*TIME];
    int exited_process = 0;

    task *current_task;
    task *task_queue;
    
    memset(&current_task, 0, sizeof(current_task));
    memset(&task_queue, 0, sizeof(task_queue));
    memset(&result, -1, sizeof(result));

    copy_origin();

    int i, j;
    for(i=0; i < TASK_SIZE*TIME; ++i) {  
        if(exited_process == num_of_task) { // 모든 프로세스가 종료되었을 때
            break;
        }

        for(j=0; j < num_of_task; ++j) {
            if(process[j].arrive_time == i && process[j].task_state == NONE) {
                process[j].task_state = READY;
            }

            if(task_queue == NULL) {
                if(process[j].task_state == READY) {
                    task_queue = &process[j]; 
                } else {
                    continue;
                }

            } else { // task_queue에 값이 있을 경우
                if(task_queue->service_time > process[j].service_time && process[j].task_state == READY) {
                    task_queue = &process[j];
                }
            }
        }

        if(current_task == NULL) {
            current_task = task_queue;
            current_task->task_state = RUNNING;

            waiting_time += (i-current_task->arrive_time); // waiting_time 계산

            memset(&task_queue, 0, sizeof(task_queue));
        }

        if(current_task != NULL && current_task->remain_time > 0) {
            int remain = --current_task->remain_time;
            result[i] = current_task->index; 

            if(remain == 0) {
                current_task->task_state = EXIT; 
                exited_process++;

                memset(&current_task, 0, sizeof(current_task));
            }
        }
    }

    cpu_time = i;
    waiting_time /= num_of_task;
    turnaround_time = get_turnaroundTime(cpu_time, result);

    printf("[SJF]\n");
    draw_graph(cpu_time, result);

    printf("CPU TIME: %d\n", cpu_time);
    printf("AVERAGE TURNAROUND TIME : %.2lf\n", turnaround_time);
    printf("AVERAGE WAITING TIME : %.2lf\n\n\n", waiting_time);
}



/* shortest-remaining-time-first scheduling */
void SRT(task process[TASK_SIZE]) { 
    int cpu_time = 0;
    float turnaround_time = 0;
    float waiting_time = 0;
    
    int result[TASK_SIZE*TIME];
    int exited_process = 0;

    task *current_task;
    task *task_queue;
    
    memset(&current_task, 0, sizeof(current_task));
    memset(&task_queue, 0, sizeof(task_queue));
    memset(&result, -1, sizeof(result));

    copy_origin();

    int i, j;
    for(i=0; i < TASK_SIZE*TIME; ++i) {  
        if(exited_process == num_of_task) { // 모든 프로세스가 종료되었을 때
            break;
        }

        for(j=0; j < num_of_task; ++j) {
            if(process[j].arrive_time == i && process[j].task_state == NONE) {
                process[j].task_state = READY;
            }

            if(task_queue == NULL) {
                if(process[j].task_state == READY || process[j].task_state == WAITING) {
                    task_queue = &process[j]; 
                } else {
                    continue;
                }

            } else { // task_queue에 값이 있을 경우
                if(task_queue->remain_time > process[j].remain_time && (process[j].task_state == READY || process[j].task_state == WAITING)) {
                    task_queue = &process[j];
                } else {
                    continue;
                }
            }
        }

        if(task_queue != NULL) {
            if(current_task == NULL) {
                current_task = task_queue;
                current_task->task_state = RUNNING;

                memset(&task_queue, 0, sizeof(task_queue));

            } else { // 현재 작업중인 일이 있을 때
                if(current_task->remain_time > task_queue->remain_time) {
                    current_task->task_state = WAITING;
                    current_task = task_queue;
                    current_task->task_state = RUNNING;

                    memset(&task_queue, 0, sizeof(task_queue));
                }
            }
        }

        if(current_task != NULL && current_task->remain_time > 0) {
            int remain = --current_task->remain_time;
            result[i] = current_task->index; 

            if(remain == 0) {
                current_task->task_state = EXIT; 
                exited_process++;

                memset(&current_task, 0, sizeof(current_task));
            }
        }
    }

    cpu_time = i;
    waiting_time = get_waitingTime(cpu_time, result);
    turnaround_time = get_turnaroundTime(cpu_time, result);

    printf("[SRT]\n");
    draw_graph(cpu_time, result);

    printf("CPU TIME: %d\n", cpu_time);
    printf("AVERAGE TURNAROUND TIME : %.2lf\n", turnaround_time);
    printf("AVERAGE WAITING TIME : %.2lf\n\n\n", waiting_time);
}


/* round-robin scheduling */
void RR(task process[TASK_SIZE]) { 
    int cpu_time = 0;
    float turnaround_time = 0;
    float waiting_time = 0;

    int result[TASK_SIZE*TIME];
    int exited_process = 0;

    memset(&result, -1, sizeof(result));

    copy_origin();
    initList();

    int i, j;
    for(i=0; i < TASK_SIZE*TIME; ++i) { 
        if(exited_process == num_of_task) { // 모든 프로세스가 종료되었을 때
            break;
        }

        for(j=0; j < num_of_task; ++j) {
            if(process[j].arrive_time == i && process[j].task_state == NONE) {
                process[j].task_state = READY;
                if(head == NULL) {
                    head->value = j;
                } else {
                    addNodeFirst(j); 
                }
            }
        }

        if(head != NULL) {
            int process_num = head->value;
            process[process_num].remain_time--;
            result[i] = process[process_num].index;

            if(process[process_num].remain_time == 0) {
                process[process_num].task_state = EXIT; 
                exited_process++;

                removeNode();

            } else {
                removeNode();

                addNode(process_num);
            }
        }
    }

    cpu_time = i;
    waiting_time = get_waitingTime(cpu_time, result);
    turnaround_time = get_turnaroundTime(cpu_time, result);

    printf("[RR]\n");
    draw_graph(cpu_time, result);

    printf("CPU TIME: %d\n", cpu_time);
    printf("AVERAGE TURNAROUND TIME : %.2lf\n", turnaround_time);
    printf("AVERAGE WAITING TIME : %.2lf\n\n\n", waiting_time);
}


/* priority scheduling(preemptive) */
void PR(task process[TASK_SIZE]) { 
    int cpu_time = 0;
    float turnaround_time = 0;
    float waiting_time = 0;
    
    int result[TASK_SIZE*TIME];
    int exited_process = 0;

    task *current_task;
    task *task_queue;
    
    memset(&current_task, 0, sizeof(current_task));
    memset(&task_queue, 0, sizeof(task_queue));
    memset(&result, -1, sizeof(result));

    copy_origin();

    int i, j;
    for(i=0; i < TASK_SIZE*TIME; ++i) {  
        if(exited_process == num_of_task) { // 모든 프로세스가 종료되었을 때
            break;
        }

        for(j=0; j < num_of_task; ++j) {
            if(process[j].arrive_time == i && process[j].task_state == NONE) {
                process[j].task_state = READY;
            }

            if(task_queue == NULL) {
                if(process[j].task_state == READY || process[j].task_state == WAITING) {
                    task_queue = &process[j]; 
                } else {
                    continue;
                }

            } else { // task_queue에 값이 있을 경우
                if(task_queue->priority > process[j].priority && (process[j].task_state == READY || process[j].task_state == WAITING)) {
                    task_queue = &process[j];
                } else {
                    continue;
                }
            }
        }

        if(task_queue != NULL) {
            if(current_task == NULL) {
                current_task = task_queue;
                current_task->task_state = RUNNING;

                memset(&task_queue, 0, sizeof(task_queue));

            } else { // 현재 작업중인 일이 있을 때
                if(current_task->priority > task_queue->priority) {
                    current_task->task_state = WAITING;
                    current_task = task_queue;
                    current_task->task_state = RUNNING;

                    memset(&task_queue, 0, sizeof(task_queue));
                }
            }
        }

        if(current_task != NULL && current_task->remain_time > 0) {
            int remain = --current_task->remain_time;
            result[i] = current_task->index; 

            if(remain == 0) {
                current_task->task_state = EXIT; 
                exited_process++;

                memset(&current_task, 0, sizeof(current_task));
            }
        }
    }

    cpu_time = i;
    waiting_time = get_waitingTime(cpu_time, result);
    turnaround_time = get_turnaroundTime(cpu_time, result);

    printf("[PR]\n");
    draw_graph(cpu_time, result);

    printf("CPU TIME: %d\n", cpu_time);
    printf("AVERAGE TURNAROUND TIME : %.2lf\n", turnaround_time);
    printf("AVERAGE WAITING TIME : %.2lf\n", waiting_time);
}


int config_read(char *filename) {
    FILE *fp;
    char config[COMMAND_SIZE];
    int line = 0;
    task temp_task;

    fp = fopen(filename, "r");
    
    if(fp != NULL) {
        while(fgets(config, sizeof(config), fp)) {
            line++;

            if(config[0] == '\n' || config[0] == '#' || config[0] == '\0') { // 주석이나 공백일 경우
                continue;

            } else {
                sscanf(config, "%s %d %d %d\n", temp_task.id, &temp_task.arrive_time, &temp_task.service_time, &temp_task.priority);
            }

            /* Error Handling */
            if(check_valid_id(temp_task.id) == -1) {
                fprintf(stderr, "invalid process id '%s' in line %d, ignored\n", temp_task.id, line); 
                continue;

            }

            if(check_duplicated_id(temp_task.id) == -1) {
                fprintf(stderr, "duplicate process id '%s' in line %d, ignored\n", temp_task.id, line); 
                continue;

            }

            if(check_arrive_time(temp_task.arrive_time) == -1) {
                fprintf(stderr, "invalid arrive-time '%d' in line %d, ignored\n", temp_task.arrive_time, line); 
                continue;

            }
            
            if(check_service_time(temp_task.service_time) == -1) {
                fprintf(stderr, "invalid service-time '%d' in line %d, ignored\n", temp_task.service_time, line); 
                continue;

            }

            if(check_priority(temp_task.priority) == -1) {
                fprintf(stderr, "invalid priority '%d' in line %d, ignored\n", temp_task.priority, line); 
                continue;
            }

            tasks[num_of_task].index = num_of_task;
            strcpy(tasks[num_of_task].id, temp_task.id);
            tasks[num_of_task].arrive_time = temp_task.arrive_time;
            tasks[num_of_task].service_time = temp_task.service_time;
            tasks[num_of_task].priority = temp_task.priority;
            tasks[num_of_task].task_state = NONE;
            tasks[num_of_task].remain_time = temp_task.service_time;

            num_of_task++;

        }

        fclose(fp);

        int n;
        for(n=0; n < TASK_SIZE; ++n) {
            origin_tasks[n] = tasks[n];
        }

    } else {
        fprintf(stderr, "failed to load input file '%s'\n", filename); // 잘못된 입력파일을 지정한 경우

        return -1;
    }

    return 0;
}


int main (int argc, char **argv) {
    if(argc <= 1) { // input 파일을 지정하지 않았을 경우, error handling
        fprintf(stderr, "input file must specified\n");
        return -1;
    }

    config_read(argv[1]);
    printf("\n");

    SJF(tasks);
    SRT(tasks);
    RR(tasks);
    PR(tasks);
  
    return 0;
}
