import { Switch, Route } from "wouter";
import { queryClient } from "./lib/queryClient";
import { QueryClientProvider } from "@tanstack/react-query";
import { Toaster } from "@/components/ui/toaster";
import { TooltipProvider } from "@/components/ui/tooltip";
import { ThemeProvider } from "@/components/theme-provider";
import Master from "@/pages/master";
import SlaveControl from "@/pages/slave-control";
import BoardSelector from "@/pages/board-selector";
import NotFound from "@/pages/not-found";

function Router() {
  return (
    <Switch>
      <Route path="/board-selector" component={BoardSelector} />
      <Route path="/slave/:id">
        {(params) => <SlaveControl stationId={parseInt(params.id)} />}
      </Route>
      <Route path="/master" component={Master} />
      <Route path="/" component={BoardSelector} />
      <Route component={NotFound} />
    </Switch>
  );
}

function App() {
  return (
    <ThemeProvider defaultTheme="light" storageKey="charging-station-theme">
      <QueryClientProvider client={queryClient}>
        <TooltipProvider>
          <Toaster />
          <Router />
        </TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>
  );
}

export default App;
