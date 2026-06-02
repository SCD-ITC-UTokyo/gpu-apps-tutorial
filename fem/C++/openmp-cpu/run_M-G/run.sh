#PBS -q debug-g
#PBS -l select=1:mpiprocs=1:ompthreads=72
#PBS -l walltime=00:05:00
#PBS -W group_list=xxxx
#PBS -j oe

# ------------ program execution --- #
module purge
module load cuda/12.9
module load nvidia/25.11
module list

cd $PBS_O_WORKDIR

export OMP_NUM_THREADS=72
time -p ../solcpp

