
void Field_Solver_3d()
{
    float T, Nt, c, dt, x, P, alpha, beta, M;
    float nu, vL, vR, N;
    T = 10;
    c = 1;
    dt = .1;
    dx = .1;
    P = 1;
    beta = 2; // need to put "get_beta" in here...
    M = 4;
    alpha = beta/(c*dt);
    
    x = -1:dx:1; // need to find out what this means
    nu = alpha*diff(x); // is there a function called "diff" that takes "x: as its argument
    vL = exp(-alpha*(x - x(1))); // ??
    vR = exp(-alpha*(x(end)-x)); // ??
    Nt = T/dt;
    N = length(x);
    
    % Initial conditions, initialize MOLT weights
    [wL,wR] = Get_Exp_Weights(nu,M);
    phi_i = @(x,y,z) exp(- (x.^2+y.^2+z.^2)/36);
    Ax_i = @(x,y,z) exp(- (x.^2+y.^2+z.^2)/36);
    Ay_i = @(x,y,z) exp(- (x.^2+y.^2+z.^2)/36);
    Az_i = @(x,y,z) exp(- (x.^2+y.^2+z.^2)/36);
    
    % Fields for storage
        phi0= zeros(N,N,N);
    phi1= zeros(N,N,N);
    phi2= zeros(N,N,N);
    Ax0    = zeros(N,N,N);
    Ax1    = zeros(N,N,N);
    Ax2    = zeros(N,N,N);
    Ay0    = zeros(N,N,N);
    Ay1    = zeros(N,N,N);
    Ay2    = zeros(N,N,N);
    Az0    = zeros(N,N,N);
    Az1    = zeros(N,N,N);
    Az2    = zeros(N,N,N);
    
    % First time steps:  n = 0, 1.
    for nx = 1:N
        for ny = 1:N
            for nz = 1:N
                xi    = x(nx);
    yj    = x(ny);  // might want to add in y, z later?
    zk    = x(nz);
    phi0(nx,ny,nz)= phi_i(xi,yj,zk);
    Ax0(nx,ny,nz) = Ax_i(xi,yj,zk);
    Ay0(nx,ny,nz) = Ay_i(xi,yj,zk);
    Az0(nx,ny,nz) = Az_i(xi,yj,zk);
    phi1(nx,ny,nz)= phi_i(xi,yj,zk);
    Ax1(nx,ny,nz) = Ax_i(xi,yj,zk);
    Ay1(nx,ny,nz) = Ay_i(xi,yj,zk);
    Az1(nx,ny,nz) = Az_i(xi,yj,zk);
    end
    end
    end
    
    
    for(nt=2; nt<Nt; nt++) // for nt = 2:Nt
        
        % ___________Do each field update (can be made parallel)___________ %
        % ___________z sweep_________%
        phi2 = do_zsweep(phi1,nu,M,wL,wR,vL,vR);
    Ax2 = do_zsweep(Ax1,nu,M,wL,wR,vL,vR);
    Ay2 = do_zsweep(Ay1,nu,M,wL,wR,vL,vR);
    Az2 = do_zsweep(Az1,nu,M,wL,wR,vL,vR);
    for(nx=1; nx<N; nx++)      // for nx = 1:N
        %         for ny = 1:N
            %             u = phi1(nx,ny,:);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nz = 1:N
        %                 phi2(nx,ny,nz) = w(nz);
    %             end
    %             %
    %             u = Ax1(nx,ny,:);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nz = 1:N
        %                 Ax2(nx,ny,nz) = w(nz);
    %             end
    %             %
    %             u = Ay1(nx,ny,:);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nz = 1:N
        %                 Ay2(nx,ny,nz) = w(nz);
    %             end
    %             %
    %             u = Az1(nx,ny,:);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nz = 1:N
        %                 Az2(nx,ny,nz) = w(nz);
    %             end
    %         end
    %     end
    
    % ___________y sweep_________%
    phi2 = do_ysweep(phi2,nu,M,wL,wR,vL,vR);
    Ax2 = do_ysweep(Ax2,nu,M,wL,wR,vL,vR);
    Ay2 = do_ysweep(Ay2,nu,M,wL,wR,vL,vR);
    Az2 = do_ysweep(Az2,nu,M,wL,wR,vL,vR);
    %     for nx = 1:N
        %         for nz = 1:N
            %             u = phi2(nx,:,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for ny = 1:N
        %                 phi2(nx,ny,nz) = w(ny);
    %             end
    %             %
    %             u = Ax2(nx,:,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for ny = 1:N
        %                 Ax2(nx,ny,nz) = w(ny);
    %             end
    %             %
    %             u = Ay2(nx,:,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for ny = 1:N
        %                 Ay2(nx,ny,nz) = w(ny);
    %             end
    %             %
    %             u = Az2(nx,:,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for ny = 1:N
        %                 Az2(nx,ny,nz) = w(ny);
    %             end
    %         end
    %     end
    
    % ___________x sweep_________%
    phi2 = do_xsweep(phi2,nu,M,wL,wR,vL,vR);
    Ax2 = do_xsweep(Ax2,nu,M,wL,wR,vL,vR);
    Ay2 = do_xsweep(Ay2,nu,M,wL,wR,vL,vR);
    Az2 = do_xsweep(Az2,nu,M,wL,wR,vL,vR);
    %     for ny = 1:N
        %         for nz = 1:N
            %             u = phi2(:,ny,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nx = 1:N
        %                 phi2(nx,ny,nz) = w(nx);
    %             end
    %             %
    %             u = Ax2(:,ny,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nx = 1:N
        %                 Ax2(nx,ny,nz) = w(nx);
    %             end
    %             %
    %             u = Ay2(:,ny,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nx = 1:N
        %                 Ay2(nx,ny,nz) = w(nx);
    %             end
    %             %
    %             u = Az2(:,ny,nz);
    %             w = Make_L(u,nu,M,wL,wR,vL,vR);
    %             for nx = 1:N
        %                 Az2(nx,ny,nz) = w(nx);
    %             end
    %         end
    %     end
    
    % ___________time update_________%
    phi2= phi2 +2*phi1 - phi0;
    Ax2    = Ax2  +2*Ax1  - Ax0;
    Ay2    = Ay2  +2*Ay1  - Ay0;
    Az2    = Az2  +2*Az1  - Az0;
    
    % ___________add sources_________%
    
    % ___________time swap_________%
    phi0= phi1;
    phi1= phi2;
    Ax0    = Ax1;
    Ax1    = Ax2;
    Ay0    = Ay1;
    Ay1    = Ay2;
    Az0    = Ax1;
    Az1    = Ax2;
    
    
    toc;
    disp(['nt = ' num2str(nt)]);
    end
}
