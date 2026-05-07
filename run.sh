make clean 
make
# LD_DEBUG=libs  
LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH   LD_PRELOAD=./inject_lib.so   llama-perplexity  -t 1  -m ~/yangzheng/llama_cpp_debug/models/tinyllama-1.1b-chat.Q4_K_M.gguf -f file.txt