#
# Passar os argumentos para esse script em ordem crescente. Caso contrario,
# o plot do throughtput X conn_interval nao fica legal.
# 

library("stringr")

# Alpha value used in all analysis
alpha <- 0.05

plot_hist <- function(name, data) {
	title <- paste("Histogram", name)

	hist(data, freq=TRUE, main=title, xlab="nr_reads", col="lightblue",
		 														border="pink")
}

plot_boxplot <- function(name, data) {
	title <- paste("Boxplot", name)

	boxplot(data, main=title, ylab="nr_reads", col="lightblue")
}

plot_time_series <- function(name, data) {
	title <- paste("Time Series", name)

	plot(data, type="p", main=title, ylab="nr_reads")
}

descriptive <- function(name, data) {
	cat(name, "\n")
	cat("\tMedia da amostra:\t", mean(data), "\n")
	cat("\tMediana da amostra:\t", median(data), "\n")
	cat("\tDesvio Padrao:\t\t", sd(data), "\n")
	cat("\tAmplitute:\t\t", max(data) - min(data), "\n")
	cat("\n")

	plot_hist(name, data)
	plot_boxplot(name, data)
	#plot_time_series(name, data)
}

main <- function() {
	argv <- commandArgs(trailingOnly=TRUE)
	argc <- length(argv)
	num_reads <- array()
	num_writers_threads <- array()

	cat("\n* Using alpha =", alpha, "\n\n")

	for (i in seq(argc)) {
		filename <- argv[i]
		table <- read.table(filename, header=TRUE, sep="\t")
		data <- table$nr_reads

		descriptive(filename, data)

		test <- wilcox.test(data, conf.level=(1-alpha), conf.int=TRUE)
		num_reads[i] <- test$estimate

		num <- str_extract(filename, "[0-9]+")
		num_writers_threads[i] <- num
		cat("\n")
	}

	plot(type="o", x=num_writers_threads, xlab="# of writer threads",
		 			y=num_reads, ylab="# of reads operations", col="darkblue")
}

main()
