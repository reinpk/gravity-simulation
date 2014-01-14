% Set paramaters
collisions = 0;
steps = dataIn(1,5);
particlecount = dataIn(1,2);
dat = dataIn(3:particlecount*steps+2, :);
stepsegment = 1;%%steps/50;
printsteps = [stepsegment:(stepsegment):steps];
position = 1;
frequency = [0];
remainingparticles = particlecount;
liveparticles = squeeze(zeros(steps-1,1));
accreted_mass = squeeze(zeros(steps-1,1));
total_mass = sum(dat(1:2998,5));
total_accreted = squeeze(zeros(steps-1,1));
for C = 0 : steps - 2
	for R = 1 : particlecount
		if dat(R + particlecount*C, 1) > dat(R + particlecount*(C + 1), 1)
			collisions = collisions + 1;
            accreted_mass(C+1) = accreted_mass(C+1) + dat(R + particlecount*C, 5);
        end
        if dat(R + particlecount*C,1) == 1
            liveparticles(C+1,1) = liveparticles(C+1,1) + 1;
        end
		if C == printsteps(1, position)
			frequency(position, 1) = collisions;
			position = position + 1;
			collisions = 0;
        end
    end
    total_accreted(C+1) = sum(accreted_mass);
end
figure(1);
plot(frequency);
figure(2);
plot(liveparticles);
figure(3);
plot(total_mass./liveparticles);

%%radius vs. mass movie
figure(4);
scatter = [0,0;0,0];
max_mass = 0;
for K = 0 : steps - 1
    for I = 1 : particlecount
        vec = dat(K*particlecount + I,2:4);
        radius = ((vec)*(vec)')^(.5);
        if dat(I + K*particlecount,1) == 1
            if dat(I + K*particlecount,5) < 300
                if dat(I + K*particlecount,5) > max_mass
                    max_mass = dat(I + K*particlecount,5);
                end
                scatter(I,1)= radius;
                scatter(I,2)= dat(I + K*particlecount,5);
            end
        end
    end
    plot(log(scatter(:,1)), scatter(:,2),'r.');
    scatter=[0,0;0,0];
    pause(0.05);
end

%%distribution of masses plot
bins = 30;
mass_freq1 = squeeze(zeros(bins,1));
mass_freq2 = squeeze(zeros(bins,1));
mass_freq_x = [max_mass/(2*bins):max_mass/bins:(max_mass - max_mass/(2*bins))];
radial_vel = squeeze(zeros(particlecount,1));
radius = squeeze(zeros(particlecount,1));
for I = 1 : particlecount
    if dat(I,1) == 1
        m = dat(I,5);
        if m < 300 && m > 0
            mass_freq1(ceil(bins*m/max_mass)) = mass_freq1(ceil(bins*m/max_mass)) + 1;
        end
        r2 = dat(I +(steps-1),2:4);
        r1 = dat(I +(steps-2),2:4);
        radial_vel(I) = (((r2+r1)/2)/norm((r2+r1)/2))*(r2-r1)';
        radius(I) = norm(r2);
    end
    if dat(I + (steps - 1)*particlecount,1) == 1
        m = dat(I + (steps-1)*particlecount,5);
        if m < 300 && m > 0
            mass_freq2(ceil(bins*m/max_mass)) = mass_freq2(ceil(bins*m/max_mass)) + 1;
        end
    end
end
figure(5)
plot(mass_freq_x, mass_freq1,'b.:');
hold on;
plot(mass_freq_x, mass_freq2,'r.-');
hold off;
figure(6)
plot(radius, radial_vel,'r.');




