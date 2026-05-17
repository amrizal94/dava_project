module.exports = {
  apps: [
    {
      name: "dava-monitoring-dev",
      script: "uvicorn",
      args: "api.main:app --reload --host 0.0.0.0 --port 8002",
      cwd: "./server",
      interpreter: "none",
      env: { PYTHONUNBUFFERED: "1" },
      profiles: ["dev"],
    },
    {
      name: "dava-monitoring-prod",
      script: "uvicorn",
      args: "api.main:app --host 127.0.0.1 --port 8002 --workers 2",
      cwd: "/www/wwwroot/monitoring.kreasikaryaarjuna.co.id/server",
      interpreter: "none",
      env: { PYTHONUNBUFFERED: "1" },
      out_file: "/www/wwwlogs/dava-monitoring.out.log",
      error_file: "/www/wwwlogs/dava-monitoring.error.log",
      profiles: ["prod"],
    },
  ],
};
