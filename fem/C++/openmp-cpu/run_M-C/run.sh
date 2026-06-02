#PBS -q debug-c
#PBS -l select=1:mpiprocs=1:ompthreads=112
#PBS -l walltime=00:05:00
#PBS -W group_list=xxxx
#PBS -j oe

# ------------ program execution --- #
module purge
module load intel/2025.2.0
module list

cd $PBS_O_WORKDIR

export OMP_NUM_THREADS=112
time -p ../solcpp

