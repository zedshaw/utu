require(R2HTML)

runchart <- function(x, main, ylab, xlab) {
  plot(x, type="l", col="gray", main=main, ylab=ylab, xlab=xlab)
  abline(h=mean(x), col="black")
  abline(h=mean(x)+2*sd(x), col="red")
  abline(h=mean(x)+sd(x), col="orange")
}

stats <- read.table("build_logs/qualstats.log", header=T)
attach(stats)

width <- 800
height <- 800
total <- verrors+unit+efw

png("build_logs/errors.png", width=width, height=height, quality=100)
par(mfrow=c(2,2))
runchart(verrors, "Memory Errors", "count", "build")
runchart(vmemuse/1024, "Memory Leaks", "kbytes", "build")
runchart(unit, "Unit Test Errors", "count", "build")
runchart(efw, "Logged Test Errors", "count", "build")
dev.off()


png("build_logs/sloc.png", width=width, height=height, quality=100)
plot(sloc, main="SLOC & SVN Diff", ylab="sloc & svn diff", xlab="build", type="l", col="orange", ylim=c(0,max(sloc+sdiff)))
lines(sdiff, col="salmon")
abline(h=mean(sloc), col="gray")
abline(h=mean(sdiff), col="gray")
legend(x=max(sdiff+sloc)-100, lty=c(1,1), legend=c("SLOC","SVN"), col=c("orange","salmon"))
dev.off()

png("build_logs/sloc_by_total.png", width=width, height=height, quality=100)
par(mfrow=c(2,1))
runchart(sloc / (total+1), "SLOC / Total Errors", "lines / total", "build")
runchart(sdiff / (total+1), "SVN Diff / Total Errors", "lines / total", "build")
dev.off()

HTMLStart("build_logs", HTMLframe=F, Title="Utu Defect Report", autobrowse=FALSE)
as.title("Utu Summary Statistics")
summary(stats)

as.title("Total ~ SVN Diff")
summary(lm(total ~ sdiff - 1))
as.title("Error/Fail Log ~ SVN Diff")
summary(lm(efw ~ sdiff - 1))
as.title("Unit Errors ~ SVN Diff")
summary(lm(unit ~ sdiff - 1))
as.title("Valgrind Errors ~ SVN Diff")
summary(lm(verrors ~ sdiff - 1))
as.title("Valgrind Memory Leaks ~ SVN Diff")
summary(lm(vmemuse ~ sdiff - 1))


as.title("Total ~ SLOC")
summary(lm(total ~ sloc - 1))
as.title("Error/Fail Log ~ SLOC")
summary(lm(efw ~ sloc - 1))
as.title("Unit Errors ~ SLOC")
summary(lm(unit ~ sloc - 1))
as.title("Valgrind Errors ~ SLOC")
summary(lm(verrors ~ sloc - 1))
as.title("Valgrind Memory Leaks ~ SLOC")
summary(lm(vmemuse ~ sloc - 1))

as.title("Utu Defect Graphs")
HTMLInsertGraph("sloc_by_total.png", Caption="SLOC by Total Defects per Build", WidthHTML=width, HeightHTML=height)
HTMLInsertGraph("sloc.png", Caption="SLOC and SVN Diff per Build", WidthHTML=width, HeightHTML=height)
HTMLInsertGraph("errors.png", Caption="Error Run Charts per Build", WidthHTML=width, HeightHTML=height)

HTMLStop()
