# Clic relatif a la fenetre Brave Frontier (robuste au deplacement de la fenetre).
# Usage: .\click.ps1 <fracX> <fracY>   (fractions 0..1 de la zone de jeu)
param([double]$fx, [double]$fy)
Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class Clk2 {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h,int c);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, IntPtr e);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
  public static IntPtr hwnd=IntPtr.Zero; public static RECT rect;
  static bool Cb(IntPtr h, IntPtr l){ if(!IsWindowVisible(h))return true; var sb=new StringBuilder(256);GetWindowText(h,sb,256); if(sb.ToString().IndexOf("Brave Frontier",StringComparison.OrdinalIgnoreCase)>=0){hwnd=h;} return true;}
  public static int[] Click(double fx,double fy){
    EnumWindows((h,l)=>Cb(h,l), IntPtr.Zero);
    if(hwnd==IntPtr.Zero) return new int[]{-1,-1};
    SetForegroundWindow(hwnd);
    System.Threading.Thread.Sleep(400);
    GetWindowRect(hwnd, out rect);
    int x=rect.L+(int)((rect.R-rect.L)*fx);
    int y=rect.T+(int)((rect.B-rect.T)*fy);
    SetCursorPos(x,y); System.Threading.Thread.Sleep(150);
    mouse_event(0x0002,0,0,0,IntPtr.Zero); System.Threading.Thread.Sleep(60); mouse_event(0x0004,0,0,0,IntPtr.Zero);
    return new int[]{x,y};
  }
}
"@
$r=[Clk2]::Click($fx,$fy)
Write-Output "click @ screen $($r[0]),$($r[1])"
