#PBS -q debug-c
#PBS -l select=1:mpiprocs=1:ompthreads=112
#PBS -l walltime=00:10:00
#PBS -W group_list=xxx
#PBS -j oe

module purge
module load intel/2025.2.0
module list

cd ${PBS_O_WORKDIR}

export OMP_PROC_BIND=spread
export OMP_PLACES=threads

time -p ./C2.out
