#!/bin/sh
set -e

docker compose down --remove-orphans
docker compose up -d --build

sleep 1

tmux new-session -d -s deployment

# Top left (pane 0)
tmux send-keys -t deployment "docker compose exec calculator_service /etc/threesomeip/bin/runtime" C-m

# Bottom left (pane 1)
tmux split-window -v -t deployment.0
tmux send-keys -t deployment "docker compose exec calculator_service /etc/threesomeip/bin/calculator_service" C-m

# Top right (pane 2)
tmux split-window -h -t deployment.0
tmux send-keys -t deployment "docker compose exec calculator_client /etc/threesomeip/bin/runtime" C-m

# Bottom right (pane 3)
tmux split-window -v -t deployment.2
tmux send-keys -t deployment "docker compose exec calculator_client /etc/threesomeip/bin/calculator_client" C-m

tmux select-layout -t deployment tiled
tmux attach -t deployment

docker compose down --remove-orphans