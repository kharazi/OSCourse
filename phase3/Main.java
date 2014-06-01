import java.awt.Color;
import java.util.concurrent.Semaphore;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;


public class Main {
    private final int total = 100;
    private final int n = 4;
    private final int barbers = 3;

    private int customers = 0;
    private final BlockingQueue<Class<Void>> standingRoom = new ArrayBlockingQueue<Class<Void>>(16, true);
    private final BlockingQueue<Class<Void>> watingBench = new ArrayBlockingQueue<Class<Void>>(4, true);
    private final Semaphore mutex = new Semaphore(1);
    private final Semaphore chair = new Semaphore(3);
    private final Semaphore barber = new Semaphore(0);
    private final Semaphore customer = new Semaphore(0);
    private final Semaphore cash = new Semaphore(0);
    private final Semaphore receipt = new Semaphore(0);
    
    private final ExecutorService customerPool = Executors.newFixedThreadPool(n);
    private final ExecutorService barberPool = Executors.newFixedThreadPool(barbers);

    private final CountDownLatch latch = new CountDownLatch(total + 1);

    public static void main(String[] args) throws InterruptedException {
        try {
            Main obj = new Main();
            obj.start();
        }
        catch (Exception e) {
            e.printStackTrace ();
        }
    }
    
    public void start () throws Exception {
        // put your code here
 
        System.out.println ((char)27 + "[2J");
        System.out.println("\033[31mOPEN \033[0m");
        for(int i = 0; i < 3; i++)
            barberPool.submit(new Barber(i));
        
        for(int i = 0; i < total; i++)
            customerPool.submit(new Customer(i));
        
        latch.countDown();
        latch.await(total, TimeUnit.SECONDS);
        System.out.println("\033[31mCLOSE \033[0m");

    }

    private class Customer implements Runnable {
        private final int id;
        
        Customer(int id){
            this.id = id;
        }
        
        public void run() {
            try {

                mutex.acquireUninterruptibly();
                if(customers == 20 ) {
                    mutex.release();
                    exitShop();
                    return;
                }
                ++customers;
                mutex.release();
                
                standingRoom.put(Void.TYPE);
                enterShop();
                
                watingBench.put(Void.TYPE);
                sitOnWaitingBench();
                standingRoom.take();
                
                chair.acquireUninterruptibly();
                sitInBarberChair();
                watingBench.take();
                
                customer.release();
                barber.acquireUninterruptibly();
                getHairCut();
                chair.release();
                
                pay();
                cash.release();
                receipt.acquireUninterruptibly();
                
                mutex.acquireUninterruptibly();
                --customers;
                mutex.release();
                
                exitShop();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } finally {
                latch.countDown();
            }


        }

        private void pay() {
            System.out.println("\033[32m" + id + ": Customer: Pay" + "\033[0m");
        }

        private void sitInBarberChair() {
            System.out.println("\033[32m" + id +": Customer: Sit in barber chair " + "\033[0m");
        }

        private void sitOnWaitingBench() {
            System.out.println("\033[32m" + id +": Customer: Sit on waiting bench " + "\033[0m");
        }

        private void enterShop() {
            System.out.println("\033[32m" + id +": Customer: Enter shop " + "\033[0m");
        }

        private void exitShop() {
            System.out.println("\033[32m" + id +": Customer: Exit shop " + "\033[0m");
        }

        private void getHairCut() {
            System.out.println("\033[32m" + id +": Customer: " + " Get Hair Cut, Waiting customers: "+ customers + "\033[0m");
        }   

    }

    private class Barber implements Runnable {
        private final int id;
        
        public Barber(int id) {
            this.id = id;
        }

        public void run() {
            while (true) {
                customer.acquireUninterruptibly();
                barber.release();
                cutHair();
                
                cash.acquireUninterruptibly();
                acceptPayment();
                receipt.release();
            }
        }

        private void cutHair() {
            System.out.println("\033[33m"+ id + ": Barber: " + " cut hair");
        }

        private void acceptPayment() {
            System.out.println("\033[34m" + id + ": Barber: " + " accept payment");
        }   
    }
}
