#!/bin/sh

cat << EOF > set_env.sh
#!/bin/sh

export GITHUB_REF="${GITHUB_REF}"
export GITHUB_SERVER_URL="${GITHUB_SERVER_URL}"
export GITHUB_REPOSITORY="${GITHUB_REPOSITORY}"
export GITHUB_RUN_ID="${GITHUB_RUN_ID}"
EOF
