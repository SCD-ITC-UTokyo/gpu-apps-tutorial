module diffusion
  use misc
  implicit none

contains

  real(dp) function diffusion3d(nx, ny, nz, dx, dy, dz, dt, kappa, f, fn)

    integer,intent(in) :: nx, ny, nz
    real(fp),intent(in) :: dx, dy, dz, dt, kappa
    real(fp),intent(in),dimension(1:nx,1:ny,0:nz+1) :: f
    real(fp),intent(out),dimension(1:nx,1:ny,0:nz+1) :: fn
    real(fp) :: ce,cw,cn,cs,ct,cb,cc
    integer :: i,j,k

    integer :: w,e,n,s,b,t

    ce = kappa*dt/(dx*dx)
    cw = ce
    cn = kappa*dt/(dy*dy)
    cs = cn
    ct = kappa*dt/(dz*dz)
    cb = ct

    cc = 1.0 - (ce + cw + cn + cs + ct + cb)

    do concurrent(k=1:nz, j=1:ny, i=1:nx) local(w,e,n,s,b,t)
       w = -1; e = 1; n = -1; s = 1; b = -1; t = 1;
       if(i == 1)  w = 0
       if(i == nx) e = 0
       if(j == 1)  n = 0
       if(j == ny) s = 0
       if(k == 1 ) b = 0
       if(k == nz) t = 0
       fn(i,j,k) = cc * f(i,j,k) + cw * f(i+w,j,k) &
            + ce * f(i+e,j,k) + cs * f(i,j+s,k) + cn * f(i,j+n,k) &
            + cb * f(i,j,k+b) + ct * f(i,j,k+t)
    end do

!!$    do k = 1, nz
!!$       do j = 1, ny
!!$          do i = 1, nx
!!$             w = -1; e = 1; n = -1; s = 1; b = -1; t = 1;
!!$             if(i == 1)  w = 0
!!$             if(i == nx) e = 0
!!$             if(j == 1)  n = 0
!!$             if(j == ny) s = 0
!!$             if(k == 1 ) b = 0
!!$             if(k == nz) t = 0
!!$             fn(i,j,k) = cc * f(i,j,k) + cw * f(i+w,j,k) &
!!$                  + ce * f(i+e,j,k) + cs * f(i,j+s,k) + cn * f(i,j+n,k) &
!!$                  + cb * f(i,j,k+b) + ct * f(i,j,k+t)
!!$          end do
!!$       end do
!!$    end do

    diffusion3d = real(nx*ny*nz,dp)*13.0

  end function diffusion3d


  subroutine init(nx, ny, nz, dx, dy, dz, f)
    integer,intent(in) :: nx, ny, nz
    real(fp),intent(in) :: dx, dy, dz
    real(fp),intent(out),dimension(1:nx,1:ny,0:nz+1) :: f
    real(fp) :: kx,ky,kz,pi
    integer  :: i,j,k
    real(fp) :: x,y,z

    pi = acos(-1.0)
    kx = 2.0*pi
    ky = kx
    kz = kx
    do concurrent(k=0:nz+1, j=1:ny, i=1:nx) local(x,y,z)
       x = dx*(real(i-1) + 0.5)
       y = dy*(real(j-1) + 0.5)
       z = dz*(real(k-1) + 0.5)

       f(i,j,k) = 0.125*(1.0 - cos(kx*x))*(1.0 - cos(ky*y))*(1.0 - cos(kz*z))
    end do

!!$    do k = 0, nz+1
!!$       do j = 1, ny
!!$          do i = 1, nx
!!$             x = dx*(real(i-1) + 0.5)
!!$             y = dy*(real(j-1) + 0.5)
!!$             z = dz*(real(k-1) + 0.5)
!!$
!!$             f(i,j,k) = 0.125*(1.0 - cos(kx*x))*(1.0 - cos(ky*y))*(1.0 - cos(kz*z))
!!$          end do
!!$       end do
!!$    end do
  end subroutine init


  real(dp) function err(time, nx, ny, nz, dx, dy, dz, kappa, f)
    real(dp),intent(in) :: time
    integer,intent(in) :: nx, ny, nz
    real(fp),intent(in) :: dx, dy, dz, kappa
    real(fp),intent(in),dimension(1:nx,1:ny,0:nz+1) :: f
    real(fp) :: kx,ky,kz,ax,ay,az,pi,ferr
    integer  :: i,j,k
    real(fp) :: x,y,z,f0

    pi = acos(-1.0)
    kx = 2.0*pi
    ky = kx
    kz = kx

    ax = exp(-kappa*time*(kx*kx))
    ay = exp(-kappa*time*(ky*ky))
    az = exp(-kappa*time*(kz*kz))

    ferr = 0.d0

    do concurrent(k=1:nz, j=1:ny, i=1:nx) local(x,y,z,f0) reduce(+:ferr)
       x = dx*(real(i-1) + 0.5)
       y = dy*(real(j-1) + 0.5)
       z = dz*(real(k-1) + 0.5)

       f0 = 0.125*(1.0 - ax*cos(kx*x)) * (1.0 - ay*cos(ky*y)) * (1.0 - az*cos(kz*z))

       ferr = ferr + (f(i,j,k) - f0)*(f(i,j,k) - f0);
    end do

!!$    do k = 1, nz
!!$       do j = 1, ny
!!$          do i = 1, nx
!!$
!!$             x = dx*(real(i-1) + 0.5)
!!$             y = dy*(real(j-1) + 0.5)
!!$             z = dz*(real(k-1) + 0.5)
!!$
!!$             f0 = 0.125*(1.0 - ax*cos(kx*x)) * (1.0 - ay*cos(ky*y)) * (1.0 - az*cos(kz*z))
!!$
!!$             ferr = ferr + (f(i,j,k) - f0)*(f(i,j,k) - f0);
!!$          end do
!!$       end do
!!$    end do
    
    err = ferr

  end function err

end module diffusion

