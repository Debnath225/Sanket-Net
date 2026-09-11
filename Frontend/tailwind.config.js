/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,jsx}"],
  theme: {
    extend: {
      colors: {
        ink: "#07131f",
        panel: "#0c1c2a",
        cyan: "#65d4d1",
        amber: "#f1ad62",
      },
    },
  },
  plugins: [],
};
