function x = bicycle_update(x_prev, v_t, L, delta_t, dt)
    x = [
        x_prev(1) + v_t * cos(x_prev(3)) * dt;
        x_prev(2) + v_t * sin(x_prev(3)) * dt;
        x_prev(3) + ((v_t * tan(delta_t)) / L) * dt;
    ];
end