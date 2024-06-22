#include "Ball.h"
#include "SDL_image.h"
#include <iostream>
#include <cmath>

Ball::Ball(double radius, Room* room) : radius_(radius), original_radius_(radius), is_resized_(false)
{
    room_ = room;

    v_[0] = 0;
    v_[1] = 0;

    mass_ = 2; // 2kg
    coeff_of_restitution_ = 0.7;
    coeff_of_friction_ = 0.5;  // 마찰 계수 초기화

    Reset();
}

void Ball::Reset()
{
    p_[0] = radius_ + room_->left_wall_x();
    p_[1] = radius_;

    v_[0] = 0;
    v_[1] = 0;
}

void Ball::Launch(double initial_force_x, double initial_force_y)
{
    v_[0] = v_[0] + (initial_force_x / mass_);
    v_[1] = v_[1] + (initial_force_y / mass_);
}

void Ball::Update(double timestep_s)
{
    double dt = timestep_s; // seconds

    // 가속도
    double a[2];
    a[0] = 0;
    a[1] = room_->gravitational_acc_y(); // Gravity

    // Move
    p_[0] = p_[0] + dt * v_[0];
    p_[1] = p_[1] + dt * v_[1];

    // Collision with Ground
    if (p_[1] - radius_ < room_->ground_height() && v_[1] < 0)
    {
        p_[1] = radius_ + room_->ground_height();
        v_[1] = -v_[1];
        v_[1] = coeff_of_restitution_ * v_[1];
    }
    // Collision with Ceiling
    else if (p_[1] + radius_ > room_->height() && v_[1] > 0)
    {
        p_[1] = room_->height() - radius_;
        v_[1] = -v_[1];
        v_[1] = coeff_of_restitution_ * v_[1];
    }
    // Collision with Left Wall
    if (p_[0] - radius_ < room_->left_wall_x() && v_[0] < 0)
    {
        p_[0] = room_->left_wall_x() + radius_;
        v_[0] = -v_[0];
        v_[0] = coeff_of_restitution_ * v_[0];
    }
    // Collision with Right Wall
    else if (p_[0] + radius_ > room_->right_wall_x() && v_[0] > 0)
    {
        p_[0] = room_->right_wall_x() - radius_;
        v_[0] = -v_[0];
        v_[0] = coeff_of_restitution_ * v_[0];
    }

    // Collision with Fence
    double fence_x = room_->vertical_fence_pos_x();
    double fence_height = room_->vertical_fence_height();

    // Handle corner collisions
    double corner_x = fence_x;
    double corner_y = fence_height;

    // Calculate distance from ball to corner
    double dist_to_corner = sqrt(pow(p_[0] - corner_x, 2) + pow(p_[1] - corner_y, 2));

    if (dist_to_corner < radius_)
    {
        // Collision response
        double overlap = radius_ - dist_to_corner;
        double normal_x = (p_[0] - corner_x) / dist_to_corner;
        double normal_y = (p_[1] - corner_y) / dist_to_corner;

        p_[0] += overlap * normal_x;
        p_[1] += overlap * normal_y;

        double relative_velocity_x = v_[0];
        double relative_velocity_y = v_[1];

        double normal_velocity = relative_velocity_x * normal_x + relative_velocity_y * normal_y;

        v_[0] -= (1 + coeff_of_restitution_) * normal_velocity * normal_x;
        v_[1] -= (1 + coeff_of_restitution_) * normal_velocity * normal_y;
    }
    else
    {
        // Handle normal fence collisions
        if (p_[0] + radius_ > fence_x && p_[0] - radius_ < fence_x)
        {
            if (p_[1] - radius_ < fence_height)
            {
                if (v_[0] > 0) // Moving right
                {
                    p_[0] = fence_x - radius_;
                }
                else if (v_[0] < 0) // Moving left
                {
                    p_[0] = fence_x + radius_;
                }
                v_[0] = -v_[0];
                v_[0] = coeff_of_restitution_ * v_[0];
            }
        }
    }

    // Update velocity with acceleration
    v_[0] = v_[0] + dt * a[0];
    v_[1] = v_[1] + dt * a[1];

    // Apply friction if on the ground
    if (p_[1] - radius_ <= room_->ground_height())
    {
        v_[0] = v_[0] - coeff_of_friction_ * dt * v_[0];
        if (abs(v_[0]) < 0.01) // Stop if the velocity is very low
        {
            v_[0] = 0;
        }
    }
}

void Ball::Resize()
{
    if (!is_resized_)
    {
        double new_radius = radius_ * 0.5;
        double delta_radius = radius_ - new_radius;

        // Adjust the position so that the ball stays on the ground
        if (p_[1] - radius_ <= room_->ground_height())
        {
            p_[1] -= delta_radius;
        }

        radius_ = new_radius;
        is_resized_ = true;
    }
}

void Ball::ResetSize()
{
    radius_ = original_radius_;
    is_resized_ = false;
}
