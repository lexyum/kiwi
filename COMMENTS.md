# COMMENTS #

## Polling ##
If I poll on the master end of the pty waiting for input to become available, `poll` *always* returns `POLLHUP` and `POLLERR`, which is complete garbage since I know the slave end is open. Can check this properly by keeping stdin and stdout connected to the terminal on the slave and verifying directly what's going on, but I don't understand how input can be available and the slave still running yet get an error from poll.