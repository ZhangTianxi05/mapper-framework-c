g++ test/register_test.cpp grpcclient/register.cpp \
    config/config.c \
    dmi/v1beta1/api.pb.cc dmi/v1beta1/api.grpc.pb.cc \
    -I. -lgrpc++ -lgrpc -lgpr -lprotobuf -lpthread \
    -labsl_log_internal_check_op -labsl_log_internal_message -labsl_log_internal_format \
    -labsl_log_internal_globals -labsl_log_internal_proto -labsl_log_internal_nullguard \
    -labsl_synchronization -labsl_time -labsl_strings -labsl_status -labsl_base \
    -labsl_cord -labsl_cord_internal \
    -labsl_cordz_info -labsl_cordz_handle -labsl_cordz_functions -labsl_cordz_sample_token \
    -labsl_hash \
    -lyaml \
    -o register_test





export HTTP_PROXY=http://127.0.0.1:7890/
export HTTPS_PROXY=http://127.0.0.1:7890/
export NO_PROXY=localhost,127.0.0.1,192.168.49.2
minikube start --docker-env HTTP_PROXY=$HTTP_PROXY --docker-env HTTPS_PROXY=$HTTPS_PROXY --docker-env NO_PROXY=$NO_PROXY