#ifndef BALL_H
#define BALL_H

#include "Room.h"

class Ball
{
public:
    double original_radius() const { return original_radius_; }

public:
    Ball(double radius, Room* room);
    void Reset();
    void Launch(double initial_force_x, double initial_force_y);
    void Update(double timestep_s);
    void Resize();
    void ResetSize();
    double pos_x() const { return p_[0]; }
    double pos_y() const { return p_[1]; }
    double radius() const { return radius_; }
public:
    const double* velocity() const { return v_; }
    double restitution() const { return coeff_of_restitution_; }
public:
    void set_pos_x(double x) { p_[0] = x; }
    void set_pos_y(double y) { p_[1] = y; }
    void set_velocity(int index, double value) { v_[index] = value; }

private:
    double radius_;
    double original_radius_;
    bool is_resized_;
    double p_[2];
    double v_[2];
    double mass_;
    double coeff_of_restitution_;
    double coeff_of_friction_;  // 마찰 계수 추가
    Room* room_;
};

#endif // BALL_H
