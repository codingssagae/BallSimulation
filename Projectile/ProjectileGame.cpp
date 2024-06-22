#include <iostream>
#include "ProjectileGame.h"
#include "SDL_image.h"
#include "G2W.h"
#include "math.h"

extern int g_current_game_phase;
extern bool g_flag_running;
extern SDL_Renderer* g_renderer;
extern SDL_Window* g_window;
extern double g_timestep_s;

ProjectileGame::ProjectileGame()
{
    g_flag_running = true;

    // Texture
    {
        SDL_Surface* ball_surface = IMG_Load("../../Resources/ball.png");
        ball_src_rectangle_.x = 0;
        ball_src_rectangle_.y = 0;
        ball_src_rectangle_.w = ball_surface->w;
        ball_src_rectangle_.h = ball_surface->h;

        ball_texture_ = SDL_CreateTextureFromSurface(g_renderer, ball_surface);
        SDL_FreeSurface(ball_surface);
    }

    mouse_win_x_ = 0;
    mouse_win_y_ = 0;

    AddNewBall();
}

ProjectileGame::~ProjectileGame()
{
    SDL_DestroyTexture(ball_texture_);
}

void ProjectileGame::AddNewBall(bool resize)
{
    balls_.emplace_back(0.11f, &room_);
    if (resize) {
        balls_.back().Resize();
    }
    predicted_path_.clear(); // 새로운 공이 추가될 때마다 예측 경로 초기화
}

void ProjectileGame::Update()
{
    // Update balls
    for (Ball& b : balls_)
    {
        b.Update(g_timestep_s);
    }

    // 마우스 포인터를 따라 예측 경로 계산
    if (!balls_.empty())
    {
        double mouse_game_x = W2G_X(mouse_win_x_);
        double mouse_game_y = W2G_Y(mouse_win_y_);

        // 가이드 라인 벡터
        double guide_line_x = mouse_game_x - balls_.back().pos_x();
        double guide_line_y = mouse_game_y - balls_.back().pos_y();

        // 임시 공으로 예측 경로 계산
        Ball temp_ball = balls_.back();
        temp_ball.Launch(8.0 * guide_line_x, 8.0 * guide_line_y);

        predicted_path_.clear();
        double total_time = 2.0; // total simulation time
        double time_step = 0.01; // smaller time step for more accuracy
        for (double t = 0; t <= total_time; t += time_step)
        {
            temp_ball.Update(time_step);

            // 펜스와의 충돌을 체크하고 처리
            double fence_x = room_.vertical_fence_pos_x();
            double fence_height = room_.vertical_fence_height();

            // Handle normal fence collisions
            if (temp_ball.pos_x() + temp_ball.radius() > fence_x && temp_ball.pos_x() - temp_ball.radius() < fence_x)
            {
                if (temp_ball.pos_y() - temp_ball.radius() < fence_height)
                {
                    if (temp_ball.velocity()[0] > 0) // Moving right
                    {
                        temp_ball.set_pos_x(fence_x - temp_ball.radius());
                    }
                    else if (temp_ball.velocity()[0] < 0) // Moving left
                    {
                        temp_ball.set_pos_x(fence_x + temp_ball.radius());
                    }
                    temp_ball.set_velocity(0, -temp_ball.velocity()[0] * temp_ball.restitution());
                }
            }

            // Handle collisions at fence corners
            // Bottom corner
            double corner_x = fence_x;
            double corner_y = fence_height;
            double dist_to_corner = sqrt(pow(temp_ball.pos_x() - corner_x, 2) + pow(temp_ball.pos_y() - corner_y, 2));
            if (dist_to_corner < temp_ball.radius())
            {
                double overlap = temp_ball.radius() - dist_to_corner;
                double normal_x = (temp_ball.pos_x() - corner_x) / dist_to_corner;
                double normal_y = (temp_ball.pos_y() - corner_y) / dist_to_corner;

                temp_ball.set_pos_x(temp_ball.pos_x() + overlap * normal_x);
                temp_ball.set_pos_y(temp_ball.pos_y() + overlap * normal_y);

                double relative_velocity_x = temp_ball.velocity()[0];
                double relative_velocity_y = temp_ball.velocity()[1];

                double normal_velocity = relative_velocity_x * normal_x + relative_velocity_y * normal_y;

                temp_ball.set_velocity(0, relative_velocity_x - (1 + temp_ball.restitution()) * normal_velocity * normal_x);
                temp_ball.set_velocity(1, relative_velocity_y - (1 + temp_ball.restitution()) * normal_velocity * normal_y);
            }

            predicted_path_.emplace_back(temp_ball.pos_x(), temp_ball.pos_y());
        }
    }
}

void ProjectileGame::Render()
{
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_renderer); // clear the renderer to the draw color

    // Draw room_
    {
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);

        // Left Wall
        SDL_RenderDrawLine(g_renderer, G2W_X(room_.left_wall_x()),
            G2W_Y(0),
            G2W_X(room_.left_wall_x()),
            G2W_Y(room_.height()));

        // Right Wall
        SDL_RenderDrawLine(g_renderer, G2W_X(room_.right_wall_x()),
            G2W_Y(0),
            G2W_X(room_.right_wall_x()),
            G2W_Y(room_.height()));

        // Top Wall
        SDL_RenderDrawLine(g_renderer, G2W_X(room_.left_wall_x()),
            G2W_Y(room_.height()),
            G2W_X(room_.right_wall_x()),
            G2W_Y(room_.height()));

        // Bottom Wall
        SDL_RenderDrawLine(g_renderer, G2W_X(room_.left_wall_x()),
            G2W_Y(0),
            G2W_X(room_.right_wall_x()),
            G2W_Y(0));

        // Fence
        SDL_RenderDrawLine(g_renderer, G2W_X(room_.vertical_fence_pos_x()),
            G2W_Y(0),
            G2W_X(room_.vertical_fence_pos_x()),
            G2W_Y(room_.vertical_fence_height()));
    }

    // Draw Balls
    for (Ball& b : balls_)
    {
        int ball_win_x = G2W_X(b.pos_x());
        int ball_win_y = G2W_Y(b.pos_y());

        double win_radius = G2W_Scale * b.radius();

        SDL_Rect dest_rect;
        dest_rect.w = (int)(2 * win_radius);
        dest_rect.h = (int)(2 * win_radius);
        dest_rect.x = (int)(ball_win_x - win_radius);
        dest_rect.y = (int)(ball_win_y - win_radius);

        SDL_RenderCopy(g_renderer, ball_texture_, &ball_src_rectangle_, &dest_rect);
    }

    // Draw the Guide Line
    if (balls_.size() > 0)
    {
        SDL_SetRenderDrawColor(g_renderer, 255, 0, 0, 100);
        SDL_RenderDrawLine(g_renderer, G2W_X(balls_.back().pos_x()),
            G2W_Y(balls_.back().pos_y()),
            mouse_win_x_,
            mouse_win_y_);
    }

    // Draw the Predicted Path
    if (!predicted_path_.empty())
    {
        SDL_SetRenderDrawColor(g_renderer, 0, 255, 0, 255); // Green color for the predicted path
        for (const auto& point : predicted_path_)
        {
            SDL_Rect rect;
            rect.x = G2W_X(point.first);
            rect.y = G2W_Y(point.second);
            rect.w = 5;
            rect.h = 5;
            SDL_RenderFillRect(g_renderer, &rect);
        }
    }

    SDL_RenderPresent(g_renderer); // draw to the screen
}

void ProjectileGame::HandleEvents()
{
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            g_flag_running = false;
            break;

        case SDL_MOUSEBUTTONDOWN:
            // 마우스 왼쪽 버튼이 눌렸을 때
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                // 커서의 x 위치 가져오기
                mouse_win_x_ = event.button.x;
                mouse_win_y_ = event.button.y;

                double mouse_game_x = W2G_X(mouse_win_x_);
                double mouse_game_y = W2G_Y(mouse_win_y_);

                // 발사
                if (!balls_.empty())
                {
                    // 가이드 라인 벡터
                    double guide_line_x = mouse_game_x - balls_.back().pos_x();
                    double guide_line_y = mouse_game_y - balls_.back().pos_y();

                    // 발사력
                    double launching_force_x = 8.0 * guide_line_x;
                    double launching_force_y = 8.0 * guide_line_y;

                    // 발사
                    balls_.back().Launch(launching_force_x, launching_force_y);

                    // 새로운 공 추가
                    AddNewBall(balls_.back().radius() != balls_.back().original_radius());
                }
            }
            break;

        case SDL_MOUSEMOTION:
            // 커서의 x 위치 가져오기
            mouse_win_x_ = event.motion.x;
            mouse_win_y_ = event.motion.y;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_2)
            {
                if (!balls_.empty())
                {
                    balls_.back().Resize();
                }
            }
            else if (event.key.keysym.sym == SDLK_1)
            {
                if (!balls_.empty())
                {
                    balls_.back().ResetSize();
                }
            }
            break;

        default:
            break;
        }
    }
}
