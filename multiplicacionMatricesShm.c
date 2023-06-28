#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>

unsigned int sizeof_dm(int rows, int cols, size_t sizeElement){
    size_t size = rows * sizeof (void *); // index size
    size += (cols * rows * sizeElement); // Data size
    return size;
}

void create_index(void **m, int rows, int cols, size_t sizeElement){
    size_t sizeRow = cols * sizeElement;
    m[0] = m + rows;
    for (int i=1; i<rows; i++){
        m[i] = (m[i-1] + sizeRow);
    }
}

void print_matrix(int **m, int n, char *nombre){
    printf("================= Matriz %s =====================\n", nombre);
    for (int r = 0; r < n; r++){
        for (int c = 0; c < n; c++){
            printf("%d\t", m[r][c]);
        }
        printf("\n");
    }
}

int multiplicar_matriz(int **a, int **b, int n, int fil, int col){
    int resultado=0;
    for (int x = 0; x < n; x++){
        resultado += a[fil][x] * b[x][col];
    }
    return resultado;
}

int main(){
    system("clear");
    int i, np, size, n=6;

    printf("Ingrese la dimencion de la matriz: ");
    scanf("%d", &n);

    int rows = n, cols = n;
    int turno_shmid, m1_shmid ,m2_shmid, m3_shmid;
    int *turno, **A, **B, **C;
    size_t sizeMatrix = sizeof_dm(rows, cols, sizeof(int));

    turno_shmid = shmget(IPC_PRIVATE, sizeMatrix, IPC_CREAT | 0600);
    turno = shmat(turno_shmid, NULL, 0);

    m1_shmid = shmget(IPC_PRIVATE, sizeMatrix, IPC_CREAT | 0600);
    A = shmat(m1_shmid, NULL, 0);
    create_index((void*)A, rows, cols, sizeof(int));

    m2_shmid = shmget(IPC_PRIVATE, sizeMatrix, IPC_CREAT | 0600);
    B = shmat(m2_shmid, NULL, 0);
    create_index((void*)B, rows, cols, sizeof(int));

    m3_shmid = shmget(IPC_PRIVATE, sizeMatrix, IPC_CREAT | 0600);
    C = shmat(m3_shmid, NULL, 0);
    create_index((void*)C, rows, cols, sizeof(int));


    if (n%2 == 0) np = n/2;
    else np = n/2+1;

    *turno = np;
    for (i=0; i<np; i++) if (!fork()) break;

    if (i == np){
        char b[500];
        sprintf(b,"pstree -lp %d", getpid());
        system(b);
    } else sleep(3);

    if (i == np){
        sleep(1);
        int n1=1, n2=n*n;
        for (int f=0; f<n; f++)
            for (int c=0; c<n; c++){
                A[f][c] = n1++;
                B[f][c] = n2--;
            }

        printf("[%d] Padre: Estas son las matrices\n", getpid());
        print_matrix(A, n, "A");
        print_matrix(B, n, "B");
        print_matrix(C, n, "C");

        *turno = 0;
        while (*turno != i);
        printf("\n[%d] Padre: Esta es la matriz resultado\n", getpid());
        print_matrix(C, n, "C");
        // for (int a=0; a<np; a++) wait(NULL);

        shmdt(turno); shmctl(turno_shmid, IPC_RMID, 0);
        shmdt(A); shmctl(m1_shmid, IPC_RMID, 0);
        shmdt(B); shmctl(m2_shmid, IPC_RMID, 0);
        shmdt(C); shmctl(m3_shmid, IPC_RMID, 0);
    }else{
        while (*turno != i) sleep(0.5);
        printf("\n[%d] Hijo %d: Esta es la matriz resultado\n", getpid(), i+1);

        for (int c=i; c<n-i; c++){
            C[i][c] = multiplicar_matriz(A, B, n, i, c);
            C[n-i-1][c] = multiplicar_matriz(A, B, n, n-i-1, c);
        }

        for (int f=i+1; f<n-i-1; f++){
            C[f][i] = multiplicar_matriz(A, B, n, f, i);
            if (f != i) C[f][n-i-1] = multiplicar_matriz(A, B, n, f, n-i-1);
        }

        print_matrix(C, n, "C");

        if (i != n-1) *turno += 1;
        else *turno = np;

        shmdt(turno);
        shmdt(A);
        shmdt(B);
        shmdt(C);

    }


    return 0;
}
