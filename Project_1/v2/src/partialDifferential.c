#include <likwid.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "partialDifferential.h"
#include "utils.h"

#define M_PI 3.14159265358979323846
#define SQR_PI M_PI *M_PI
#define X_Y_FUNCTION(i, j) (4 * SQR_PI) * ((sin(2 * M_PI * (i)) * sinh(M_PI * (j))) + (sin(2 * M_PI * (M_PI - (i))) * (sinh(M_PI * (M_PI - (j))))))

/**
 * @brief Function to allocate space in memory.
 *
 * @param nx Number of points in x.
 * @param ny Number of points in y.
 * @return linearSystem Linear system struct.
 */
linearSystem initLinearSystem(int nx, int ny) {
    linearSystem linSys;

    linSys.ssd = (real_t *)malloc((nx * ny) * sizeof(real_t));
    memset(linSys.ssd, 0.0, (nx * ny) * sizeof(real_t));

    linSys.sd = (real_t *)malloc((nx * ny) * sizeof(real_t));
    memset(linSys.sd, 0.0, (nx * ny) * sizeof(real_t));

    linSys.md = (real_t *)malloc((nx * ny) * sizeof(real_t));
    memset(linSys.md, 0.0, (nx * ny) * sizeof(real_t));

    linSys.id = (real_t *)malloc((nx * ny) * sizeof(real_t));
    memset(linSys.id, 0.0, (nx * ny) * sizeof(real_t));

    linSys.iid = (real_t *)malloc((nx * ny) * sizeof(real_t));
    memset(linSys.iid, 0.0, (nx * ny) * sizeof(real_t));

    linSys.b = (real_t *)malloc((nx * ny) * sizeof(real_t));

    linSys.x = (real_t *)malloc((nx * ny) * sizeof(real_t));
    memset(linSys.x, 0.0, (nx * ny) * sizeof(real_t));

    linSys.nx = nx;
    linSys.ny = ny;

    return linSys;
}

/**
 * @brief Set the Linear System object
 *
 * @param linSys Linear system struct.
 */
void setLinearSystem(linearSystem *linSys) {
    real_t hx, hy, sqrHx, sqrHy;

    hx = M_PI / (linSys->nx + 1);
    hy = M_PI / (linSys->ny + 1);

    sqrHx = hx * hx;
    sqrHy = hy * hy;

    // ------------------------------------------------ FILL A DIAGONAL MATRIX ------------------------------------------------

    // Superior superior diagonal.
    for (int i = 0; i < (linSys->nx * linSys->ny) - linSys->nx; i++) {
        linSys->ssd[i] = sqrHx * (hy - 2);
    }

    // Superior diagonal.
    for (int k = 0; k < linSys->nx * linSys->ny; k++) {
        if ((k + 1) % linSys->nx != 0) {
            linSys->sd[k] = sqrHy * (hx - 2);
        }
    }

    // Main diagonal
    for (int i = 0; i < linSys->nx * linSys->ny; i++) {
        linSys->md[i] = 4 * (sqrHy + sqrHx + 2 * SQR_PI * sqrHx * sqrHy);
    }

    // Inferior diagonal
    for (int k = 0; k < linSys->nx * linSys->ny; k++) {
        if (k % linSys->nx != 0) {
            linSys->id[k] = sqrHy * (-2 - hx);
        }
    }

    // Inferior inferior diagonal
    for (int i = linSys->nx; i < linSys->nx * linSys->ny; i++) {
        linSys->iid[i] = sqrHx * (-2 - hy);
    }

    // ------------------------------------------------ FILL B ARRAY ------------------------------------------------

    int idxB = 0;

    for (int j = 1; j <= linSys->ny; j++) {
        for (int i = 1; i <= linSys->nx; i++) {
            linSys->b[idxB] = (2 * sqrHx * sqrHy) * X_Y_FUNCTION(0 + i * hx, 0 + j * hy);

            // Bottom edge of the mesh.
            if (j == 1) {
                linSys->b[idxB] -= (sin(2 * M_PI * (M_PI - i * hx)) * sinh(SQR_PI)) * (sqrHx * (-2 - hy));
            }

            // Left edge of the mesh (zero).
            if (i == 1) {
                linSys->b[idxB] -= 0;
            }

            // Upper edge of the mesh.
            if (j == linSys->ny) {
                linSys->b[idxB] -= (sin(2 * M_PI * i * hx) * sinh(SQR_PI)) * (sqrHx * (hy - 2));
            }

            // Right edge of the mesh (zero).
            if (i == linSys->nx) {
                linSys->b[idxB] -= 0;
            }

            idxB++;
        }
    }
}

/**
 * @brief Function to calculate L2 norm.
 *
 * @param linSys Linear system struct.
 * @return real_t
 */
real_t l2Norm(linearSystem *linSys) {  // Retirado os if dos for.
    real_t *temp = (real_t *)malloc((linSys->nx * linSys->ny) * sizeof(real_t));

    int i = 0;

    // primeira equação fora do laço
    temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx])) - linSys->md[i] * linSys->x[i];

    // for  ate o inicio da diagonal inferior inferior
    for (i = 1; i < linSys->nx; i++) {
        temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];
    }
    // equações com todas as diagonais
    for (; i < linSys->nx * linSys->ny - linSys->nx; i++) {
        temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) - linSys->md[i] * linSys->x[i];
    }
    // for ate o final da diagonal inferior inferior
    for (; i < (linSys->nx * linSys->ny - 1); i++) {
        temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) - linSys->md[i] * linSys->x[i];
    }
    // ultima equação fora do laço
    temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];

    real_t result = 0.0;

    for (int i = 0; i < linSys->nx * linSys->ny; i++) {
        result += temp[i] * temp[i];
    }

    return sqrt(result);
}

// real_t l2Norm(linearSystem *linSys) {  // Loop unroll de 2.
//     real_t *temp = (real_t *)malloc((linSys->nx * linSys->ny) * sizeof(real_t));
//     int aux, i = 0;

//     // primeira equação fora do laço
//     temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx])) - linSys->md[i] * linSys->x[i];

//     // for  ate o inicio da diagonal inferior inferior
//     aux = linSys->nx;
//     aux -= aux % 2;
//     for (i = 1; i < aux; i += 2) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];
//         temp[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i])) - linSys->md[i + 1] * linSys->x[i + 1];
//     }
//     for (; i < linSys->nx; i++) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];
//     }

//     // equações com todas as diagonais
//     aux = linSys->nx * linSys->ny - linSys->nx;
//     aux -= aux % 2;
//     for (; i < aux; i += 2) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) - linSys->md[i] * linSys->x[i];
//         temp[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1])) - linSys->md[i + 1] * linSys->x[i + 1];
//     }
//     for (; i < linSys->nx * linSys->ny - linSys->nx; i++) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) - linSys->md[i] * linSys->x[i];
//     }

//     // for ate o final da diagonal inferior inferior
//     aux = linSys->nx * linSys->ny - 1;
//     aux -= aux % 2;
//     for (; i < aux; i += 2) {
//         temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) - linSys->md[i] * linSys->x[i];
//         temp[i + 1] = (linSys->b[i + 1] - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->sd[i + 1] * linSys->x[i + 2])) - linSys->md[i + 1] * linSys->x[i + 1];
//     }
//     for (; i < (linSys->nx * linSys->ny - 1); i++) {
//         temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) - linSys->md[i] * linSys->x[i];
//     }

//     // ultima equação fora do laço
//     temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];

//     real_t result = 0.0;

//     // raiz dos quadrados dos residuos
//     for (i = 0; i < aux; i++) {
//         result += temp[i] * temp[i];
//     }

//     return sqrt(result);
// }

// real_t l2Norm(linearSystem *linSys) {  // Loop unroll de 4.
//     real_t *temp = (real_t *)malloc((linSys->nx * linSys->ny) * sizeof(real_t));
//     int aux, i = 0;

//     // primeira equação fora do laço
//     temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx])) - linSys->md[i] * linSys->x[i];

//     // for  ate o inicio da diagonal inferior inferior
//     aux = linSys->nx;
//     aux -= aux % 4;
//     for (i = 1; i < aux; i += 4) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];
//         temp[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i])) - linSys->md[i + 1] * linSys->x[i + 1];
//         temp[i + 2] = (linSys->b[i + 2] - (linSys->sd[i + 2] * linSys->x[i + 3]) - (linSys->ssd[i + 2] * linSys->x[i + linSys->nx + 2]) - (linSys->id[i + 2] * linSys->x[i + 1])) - linSys->md[i + 2] * linSys->x[i + 2];
//         temp[i + 3] = (linSys->b[i + 3] - (linSys->sd[i + 3] * linSys->x[i + 4]) - (linSys->ssd[i + 3] * linSys->x[i + linSys->nx + 3]) - (linSys->id[i + 3] * linSys->x[i + 2])) - linSys->md[i + 3] * linSys->x[i + 3];
//     }
//     for (; i < linSys->nx; i++) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];
//     }

//     // equações com todas as diagonais
//     aux = linSys->nx * linSys->ny - linSys->nx;
//     aux -= aux % 4;
//     for (; i < aux; i += 4) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) - linSys->md[i] * linSys->x[i];
//         temp[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1])) - linSys->md[i + 1] * linSys->x[i + 1];
//         temp[i + 2] = (linSys->b[i + 2] - (linSys->sd[i + 2] * linSys->x[i + 3]) - (linSys->ssd[i + 2] * linSys->x[i + linSys->nx + 2]) - (linSys->id[i + 2] * linSys->x[i + 1]) - (linSys->iid[i + 2] * linSys->x[i - linSys->nx + 2])) - linSys->md[i + 2] * linSys->x[i + 2];
//         temp[i + 3] = (linSys->b[i + 3] - (linSys->sd[i + 3] * linSys->x[i + 4]) - (linSys->ssd[i + 3] * linSys->x[i + linSys->nx + 3]) - (linSys->id[i + 3] * linSys->x[i + 2]) - (linSys->iid[i + 3] * linSys->x[i - linSys->nx + 3])) - linSys->md[i + 3] * linSys->x[i + 3];
//     }
//     for (; i < linSys->nx * linSys->ny - linSys->nx; i++) {
//         temp[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) - linSys->md[i] * linSys->x[i];
//     }

//     // for ate o final da diagonal inferior inferior
//     aux = linSys->nx * linSys->ny - 1;
//     aux -= aux % 4;
//     for (; i < aux; i += 4) {
//         temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) - linSys->md[i] * linSys->x[i];
//         temp[i + 1] = (linSys->b[i + 1] - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->sd[i + 1] * linSys->x[i + 2])) - linSys->md[i + 1] * linSys->x[i + 1];
//         temp[i + 2] = (linSys->b[i + 2] - (linSys->iid[i + 2] * linSys->x[i - linSys->nx + 2]) - (linSys->id[i + 2] * linSys->x[i + 1]) - (linSys->sd[i + 2] * linSys->x[i + 3])) - linSys->md[i + 2] * linSys->x[i + 2];
//         temp[i + 3] = (linSys->b[i + 3] - (linSys->iid[i + 3] * linSys->x[i - linSys->nx + 3]) - (linSys->id[i + 3] * linSys->x[i + 2]) - (linSys->sd[i + 3] * linSys->x[i + 4])) - linSys->md[i + 3] * linSys->x[i + 3];
//     }
//     for (; i < (linSys->nx * linSys->ny - 1); i++) {
//         temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) - linSys->md[i] * linSys->x[i];
//     }

//     // ultima equação fora do laço
//     temp[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) - linSys->md[i] * linSys->x[i];

//     real_t result = 0.0;

//     // raiz dos quadrados dos residuos
//     for (i = 0; i < aux; i++) {
//         result += temp[i] * temp[i];
//     }

//     return sqrt(result);
// }

/**
 * @brief Function to print the discretization matrix.
 *
 * @param linSys LinearSystem structure
 * @param output Output file.
 */
void printMesh(linearSystem *linSys, FILE *output) {
    real_t hx, hy;

    hx = M_PI / (linSys->nx + 1);
    hy = M_PI / (linSys->ny + 1);

    if (!output) {
        output = stdout;
    }

    int k = 0;

    for (int j = 1; j < linSys->ny; j++) {
        for (int i = 1; i < linSys->nx; i++) {
            fprintf(output, "%lf %lf %lf\n", i * hx, j * hy, linSys->x[k++]);
        }
    }
}

/**
 * @brief Function to print Gauss Seidel parameters.
 *
 * @param avrgTime Average time.
 * @param arrayL2Norm Array with all L2 norms.
 * @param output Output file.
 * @param it Number of iterations.
 */
void printGaussSeidelParameters(real_t avrgTime, real_t *arrayL2Norm, FILE *output, int it) {
    fprintf(output, "###########\n");

    fprintf(output, "# Tempo Método GS: %lfms\n", avrgTime);
    fprintf(output, "#\n");

    // ----------------------------------------------- L2 Norm -----------------------------------------------

    fprintf(output, "# Norma L2 do Residuo\n");

    for (int i = 0; i < it; i++) {
        fprintf(output, "#i = %d : %lf\n", i, arrayL2Norm[i]);
    }

    fprintf(output, "###########\n");
}

/**
 * @brief Gauss Seidel function.
 *
 * @param linSys Linear system struct.
 * @param it Number of max iterations.
 */
void gaussSeidel(linearSystem *linSys, int it, FILE *output) {  // Retirado os if dos for.
    real_t itTime, *arrayL2Norm, acumItTime;
    int i, k = 0;
    acumItTime = 0.0;
    arrayL2Norm = (real_t *)malloc(it * sizeof(real_t));

    LIKWID_MARKER_START("Gauss_Seidel_Likwid_Performance");
    while (k < it) {
        itTime = timestamp();
        i = 0;

        // primeira equação fora do laço
        linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx])) / linSys->md[i];

        // for  ate o inicio da diagonal inferior inferior
        for (i = 1; i < linSys->nx; i++) {
            linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];
        }
        // equações com todas as diagonais
        for (; i < linSys->nx * linSys->ny - linSys->nx; i++) {
            linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) / linSys->md[i];
        }
        // for ate o final da diagonal inferior inferior
        for (; i < (linSys->nx * linSys->ny - 1); i++) {
            linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) / linSys->md[i];
        }
        // ultima equação fora do laço
        linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];

        acumItTime += timestamp() - itTime;
        LIKWID_MARKER_START("L2_Norm_Likwid_Performance");
        arrayL2Norm[k] = l2Norm(linSys);
        LIKWID_MARKER_STOP("L2_Norm_Likwid_Performance");
        k++;
    }
    LIKWID_MARKER_STOP("Gauss_Seidel_Likwid_Performance");

    printGaussSeidelParameters(acumItTime / (it), arrayL2Norm, output, it);
}

// void gaussSeidel(linearSystem *linSys, int it, FILE *output) {  // Loop unroll de 2.
//     real_t itTime, *arrayL2Norm, acumItTime;
//     int i, aux, k = 0;
//     acumItTime = 0.0;
//     arrayL2Norm = (real_t *)malloc(it * sizeof(real_t));

//     LIKWID_MARKER_START("Gauss_Seidel_Likwid_Performance");
//     while (k < it) {
//         itTime = timestamp();
//         i = 0;

//         // primeira equação fora do laço
//         linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx])) / linSys->md[i];

//         // for  ate o inicio da diagonal inferior inferior
//         aux = linSys->nx;
//         aux -= aux % 2;
//         for (i = 1; i < aux; i += 2) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];
//             linSys->x[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i])) / linSys->md[i + 1];
//         }
//         for (; i < linSys->nx; i++) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];
//         }

//         // equações com todas as diagonais
//         aux = linSys->nx * linSys->ny - linSys->nx;
//         aux -= aux % 2;
//         for (; i < aux; i += 2) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) / linSys->md[i];
//             linSys->x[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1])) / linSys->md[i + 1];
//         }
//         for (; i < linSys->nx * linSys->ny - linSys->nx; i++) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) / linSys->md[i];
//         }
//         // for ate o final da diagonal inferior inferior
//         aux = linSys->nx * linSys->ny - 1;
//         aux -= aux % 2;
//         for (; i < aux; i += 2) {
//             linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) / linSys->md[i];
//             linSys->x[i + 1] = (linSys->b[i + 1] - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->sd[i + 1] * linSys->x[i + 2])) / linSys->md[i + 1];
//         }
//         for (; i < linSys->nx * linSys->ny - 1; i++) {
//             linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) / linSys->md[i];
//         }
//         // ultima equação fora do laço
//         linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];

//         acumItTime += timestamp() - itTime;
//         LIKWID_MARKER_START("L2_Norm_Likwid_Performance");
//         arrayL2Norm[k] = l2Norm(linSys);
//         LIKWID_MARKER_STOP("L2_Norm_Likwid_Performance");
//         k++;
//     }
//     LIKWID_MARKER_STOP("Gauss_Seidel_Likwid_Performance");

//     printGaussSeidelParameters(acumItTime / (it), arrayL2Norm, output, it);
// }

// void gaussSeidel(linearSystem *linSys, int it, FILE *output) {  // Loop unroll de 4.
//     real_t itTime, *arrayL2Norm, acumItTime;
//     int i, aux, k = 0;
//     acumItTime = 0.0;
//     arrayL2Norm = (real_t *)malloc(it * sizeof(real_t));

//     LIKWID_MARKER_START("Gauss_Seidel_Likwid_Performance");
//     while (k < it) {
//         itTime = timestamp();
//         i = 0;

//         // primeira equação fora do laço
//         linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx])) / linSys->md[i];

//         // for  ate o inicio da diagonal inferior inferior
//         aux = linSys->nx;
//         aux -= aux % 4;
//         for (i = 1; i < aux; i += 4) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];
//             linSys->x[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i])) / linSys->md[i + 1];
//             linSys->x[i + 2] = (linSys->b[i + 2] - (linSys->sd[i + 2] * linSys->x[i + 3]) - (linSys->ssd[i + 2] * linSys->x[i + linSys->nx + 2]) - (linSys->id[i + 2] * linSys->x[i + 1])) / linSys->md[i + 2];
//             linSys->x[i + 3] = (linSys->b[i + 3] - (linSys->sd[i + 3] * linSys->x[i + 4]) - (linSys->ssd[i + 3] * linSys->x[i + linSys->nx + 3]) - (linSys->id[i + 3] * linSys->x[i + 2])) / linSys->md[i + 3];
//         }
//         for (; i < linSys->nx; i++) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];
//         }

//         // equações com todas as diagonais
//         aux = linSys->nx * linSys->ny - linSys->nx;
//         aux -= aux % 4;
//         for (; i < aux; i += 4) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) / linSys->md[i];
//             linSys->x[i + 1] = (linSys->b[i + 1] - (linSys->sd[i + 1] * linSys->x[i + 2]) - (linSys->ssd[i + 1] * linSys->x[i + linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1])) / linSys->md[i + 1];
//             linSys->x[i + 2] = (linSys->b[i + 2] - (linSys->sd[i + 2] * linSys->x[i + 3]) - (linSys->ssd[i + 2] * linSys->x[i + linSys->nx + 2]) - (linSys->id[i + 2] * linSys->x[i + 1]) - (linSys->iid[i + 2] * linSys->x[i - linSys->nx + 2])) / linSys->md[i + 2];
//             linSys->x[i + 3] = (linSys->b[i + 3] - (linSys->sd[i + 3] * linSys->x[i + 4]) - (linSys->ssd[i + 3] * linSys->x[i + linSys->nx + 3]) - (linSys->id[i + 3] * linSys->x[i + 2]) - (linSys->iid[i + 3] * linSys->x[i - linSys->nx + 3])) / linSys->md[i + 3];
//         }
//         for (; i < linSys->nx * linSys->ny - linSys->nx; i++) {
//             linSys->x[i] = (linSys->b[i] - (linSys->sd[i] * linSys->x[i + 1]) - (linSys->ssd[i] * linSys->x[i + linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->iid[i] * linSys->x[i - linSys->nx])) / linSys->md[i];
//         }
//         // for ate o final da diagonal inferior inferior
//         aux = linSys->nx * linSys->ny - 1;
//         aux -= aux % 4;
//         for (; i < aux; i += 4) {
//             linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) / linSys->md[i];
//             linSys->x[i + 1] = (linSys->b[i + 1] - (linSys->iid[i + 1] * linSys->x[i - linSys->nx + 1]) - (linSys->id[i + 1] * linSys->x[i]) - (linSys->sd[i + 1] * linSys->x[i + 2])) / linSys->md[i + 1];
//             linSys->x[i + 2] = (linSys->b[i + 2] - (linSys->iid[i + 2] * linSys->x[i - linSys->nx + 2]) - (linSys->id[i + 2] * linSys->x[i + 1]) - (linSys->sd[i + 2] * linSys->x[i + 3])) / linSys->md[i + 2];
//             linSys->x[i + 3] = (linSys->b[i + 3] - (linSys->iid[i + 3] * linSys->x[i - linSys->nx + 3]) - (linSys->id[i + 3] * linSys->x[i + 2]) - (linSys->sd[i + 3] * linSys->x[i + 4])) / linSys->md[i + 3];
//         }
//         for (; i < linSys->nx * linSys->ny - 1; i++) {
//             linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1]) - (linSys->sd[i] * linSys->x[i + 1])) / linSys->md[i];
//         }
//         // ultima equação fora do laço
//         linSys->x[i] = (linSys->b[i] - (linSys->iid[i] * linSys->x[i - linSys->nx]) - (linSys->id[i] * linSys->x[i - 1])) / linSys->md[i];

//         acumItTime += timestamp() - itTime;
//         LIKWID_MARKER_START("L2_Norm_Likwid_Performance");
//         arrayL2Norm[k] = l2Norm(linSys);
//         LIKWID_MARKER_STOP("L2_Norm_Likwid_Performance");
//         k++;
//     }
//     LIKWID_MARKER_STOP("Gauss_Seidel_Likwid_Performance");

//     printGaussSeidelParameters(acumItTime / (it), arrayL2Norm, output, it);
// }
