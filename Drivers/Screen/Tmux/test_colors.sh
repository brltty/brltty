#!/bin/bash
# Test script for Tmux driver color support

# Create a temporary tmux session with colored output
SESSION="brltty_color_test"

# Kill any existing session
tmux kill-session -t "$SESSION" 2>/dev/null

# Start a new session with colored output
tmux new-session -d -s "$SESSION" -x 80 -y 24

# Send commands to display various colors
tmux send-keys -t "$SESSION" "clear" C-m
sleep 0.2

# Display color test
tmux send-keys -t "$SESSION" "printf '\033[0mDefault \033[31mRed \033[32mGreen \033[33mYellow\n'" C-m
sleep 0.2
tmux send-keys -t "$SESSION" "printf '\033[34mBlue \033[35mMagenta \033[36mCyan \033[37mWhite\n'" C-m
sleep 0.2
tmux send-keys -t "$SESSION" "printf '\033[1;31mBright Red \033[1;32mBright Green \033[1;33mBright Yellow\n'" C-m
sleep 0.2
tmux send-keys -t "$SESSION" "printf '\033[40;37mBlack BG \033[41;37mRed BG \033[42;30mGreen BG \033[43;30mYellow BG\n'" C-m
sleep 0.2
tmux send-keys -t "$SESSION" "printf '\033[44;37mBlue BG \033[45;37mMagenta BG \033[46;30mCyan BG \033[47;30mWhite BG\033[0m\n'" C-m
sleep 0.2
tmux send-keys -t "$SESSION" "printf '\033[5mBlinking text\033[0m (if supported)\n'" C-m
sleep 0.2

echo "Test session '$SESSION' created with colored output"
echo ""
echo "To test with BRLTTY, run:"
echo "  brltty -x tx -X session=$SESSION -n"
echo ""
echo "To view the session manually:"
echo "  tmux attach-session -t $SESSION"
echo ""
echo "To capture content with escape sequences:"
echo "  tmux capture-pane -t $SESSION -e -p"
echo ""
echo "To kill the session:"
echo "  tmux kill-session -t $SESSION"
